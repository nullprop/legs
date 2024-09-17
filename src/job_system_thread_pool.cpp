// Jolt Physics Library (https://github.com/jrouwe/JoltPhysics)
// SPDX-FileCopyrightText: 2021 Jorrit Rouwe
// SPDX-License-Identifier: MIT

#include <Jolt/Jolt.h>

#include <Jolt/Core/FPException.h>
#include <Jolt/Core/Profiler.h>

#include "job_system_thread_pool.hpp"

#ifdef JPH_PLATFORM_LINUX
#include <sys/prctl.h>
#endif

using namespace JPH;

namespace legs
{

void JobSystemThreadPool::Init(uint inMaxJobs, uint inMaxBarriers, int inNumThreads)
{
    JobSystemWithBarrier::Init(inMaxBarriers);

    // Init freelist of jobs
    mJobs.Init(inMaxJobs, inMaxJobs);

    // Init queue
    for (std::atomic<Job*>& j : mQueue)
    {
        j = nullptr;
    }

    // Start the worker threads
    StartThreads(inNumThreads);
}

JobSystemThreadPool::JobSystemThreadPool(uint inMaxJobs, uint inMaxBarriers, int inNumThreads)
{
    Init(inMaxJobs, inMaxBarriers, inNumThreads);
}

void JobSystemThreadPool::StartThreads([[maybe_unused]] int inNumThreads)
{
#if !defined(JPH_CPU_WASM) \
    || defined(__EMSCRIPTEN_PTHREADS__) // If we're running without threads support we cannot create
                                        // threads and we ignore the inNumThreads parameter
    // Auto detect number of threads
    if (inNumThreads < 0)
    {
        inNumThreads = std::thread::hardware_concurrency() - 1;
    }

    // If no threads are requested we're done
    if (inNumThreads == 0)
    {
        return;
    }

    // Don't quit the threads
    mQuit = false;

    // Allocate heads
    mHeads =
        reinterpret_cast<std::atomic<uint>*>(JPH::Allocate(sizeof(std::atomic<uint>) * inNumThreads)
        );
    for (int i = 0; i < inNumThreads; ++i)
    {
        mHeads[i] = 0;
    }

    // Start running threads
    JPH_ASSERT(mThreads.empty());
    mThreads.reserve(inNumThreads);
    for (int i = 0; i < inNumThreads; ++i)
    {
        mThreads.emplace_back([this, i] { ThreadMain(i); });
    }
#endif
}

JobSystemThreadPool::~JobSystemThreadPool()
{
    // Stop all worker threads
    StopThreads();
}

void JobSystemThreadPool::StopThreads()
{
    if (mThreads.empty())
    {
        return;
    }

    // Signal threads that we want to stop and wake them up
    mQuit = true;
    mSemaphore.Release((uint)mThreads.size());

    // Wait for all threads to finish
    for (std::thread& t : mThreads)
    {
        if (t.joinable())
        {
            t.join();
        }
    }

    // Delete all threads
    mThreads.clear();

    // Ensure that there are no lingering jobs in the queue
    for (uint head = 0; head != mTail; ++head)
    {
        // Fetch job
        Job* job_ptr = mQueue[head & (cQueueLength - 1)].exchange(nullptr);
        if (job_ptr != nullptr)
        {
            // And execute it
            job_ptr->Execute();
            job_ptr->Release();
        }
    }

    // Destroy heads and reset tail
    JPH::Free(mHeads);
    mHeads = nullptr;
    mTail  = 0;
}

JPH::JobHandle JobSystemThreadPool::CreateJob(
    const char*        inJobName,
    JPH::ColorArg      inColor,
    const JobFunction& inJobFunction,
    uint32_t           inNumDependencies
)
{
    JPH_PROFILE_FUNCTION();

    // Loop until we can get a job from the free list
    uint32_t index;
    for (;;)
    {
        index = mJobs.ConstructObject(inJobName, inColor, this, inJobFunction, inNumDependencies);
        if (index != AvailableJobs::cInvalidObjectIndex)
        {
            break;
        }
        JPH_ASSERT(false, "No jobs available!");
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
    Job* job = &mJobs.Get(index);

    // Construct handle to keep a reference, the job is queued below and may immediately complete
    JobHandle handle(job);

    // If there are no dependencies, queue the job now
    if (inNumDependencies == 0)
    {
        QueueJob(job);
    }

    // Return the handle
    return handle;
}

void JobSystemThreadPool::FreeJob(Job* inJob)
{
    mJobs.DestructObject(inJob);
}

uint JobSystemThreadPool::GetHead() const
{
    // Find the minimal value across all threads
    uint head = mTail;
    for (size_t i = 0; i < mThreads.size(); ++i)
    {
        head = std::min(head, mHeads[i].load());
    }
    return head;
}

void JobSystemThreadPool::QueueJobInternal(Job* inJob)
{
    // Add reference to job because we're adding the job to the queue
    inJob->AddRef();

    // Need to read head first because otherwise the tail can already have passed the head
    // We read the head outside of the loop since it involves iterating over all threads and we only
    // need to update it if there's not enough space in the queue.
    uint head = GetHead();

    for (;;)
    {
        // Check if there's space in the queue
        uint old_value = mTail;
        if (old_value - head >= cQueueLength)
        {
            // We calculated the head outside of the loop, update head (and we also need to update
            // tail to prevent it from passing head)
            head      = GetHead();
            old_value = mTail;

            // Second check if there's space in the queue
            if (old_value - head >= cQueueLength)
            {
                // Wake up all threads in order to ensure that they can clear any nullptrs they may
                // not have processed yet
                mSemaphore.Release((uint)mThreads.size());

                // Sleep a little (we have to wait for other threads to update their head pointer in
                // order for us to be able to continue)
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                continue;
            }
        }

        // Write the job pointer if the slot is empty
        Job* expected_job = nullptr;
        bool success =
            mQueue[old_value & (cQueueLength - 1)].compare_exchange_strong(expected_job, inJob);

        // Regardless of who wrote the slot, we will update the tail (if the successful thread got
        // scheduled out after writing the pointer we still want to be able to continue)
        mTail.compare_exchange_strong(old_value, old_value + 1);

        // If we successfully added our job we're done
        if (success)
        {
            break;
        }
    }
}

void JobSystemThreadPool::QueueJob(Job* inJob)
{
    JPH_PROFILE_FUNCTION();

    // If we have no worker threads, we can't queue the job either. We assume in this case that the
    // job will be added to a barrier and that the barrier will execute the job when it's Wait()
    // function is called.
    if (mThreads.empty())
    {
        return;
    }

    // Queue the job
    QueueJobInternal(inJob);

    // Wake up thread
    mSemaphore.Release();
}

void JobSystemThreadPool::QueueJobs(Job** inJobs, uint inNumJobs)
{
    JPH_PROFILE_FUNCTION();

    JPH_ASSERT(inNumJobs > 0);

    // If we have no worker threads, we can't queue the job either. We assume in this case that the
    // job will be added to a barrier and that the barrier will execute the job when it's Wait()
    // function is called.
    if (mThreads.empty())
    {
        return;
    }

    // Queue all jobs
    for (Job **job = inJobs, **job_end = inJobs + inNumJobs; job < job_end; ++job)
    {
        QueueJobInternal(*job);
    }

    // Wake up threads
    mSemaphore.Release(std::min(inNumJobs, (uint)mThreads.size()));
}

static void SetThreadName(const char* inName)
{
    JPH_ASSERT(strlen(inName) < 16); // String will be truncated if it is longer
    prctl(PR_SET_NAME, inName, 0, 0, 0);
}

void JobSystemThreadPool::ThreadMain(int inThreadIndex)
{
    // Name the thread
    char name[64];
    snprintf(name, sizeof(name), "Worker %d", int(inThreadIndex + 1));

#if defined(JPH_PLATFORM_WINDOWS) || defined(JPH_PLATFORM_LINUX)
    SetThreadName(name);
#endif // JPH_PLATFORM_WINDOWS && !JPH_COMPILER_MINGW

    // Enable floating point exceptions
    JPH::FPExceptionsEnable enable_exceptions;
    JPH_UNUSED(enable_exceptions);

    JPH_PROFILE_THREAD_START(name);

    // Call the thread init function
    mThreadInitFunction(inThreadIndex);

    std::atomic<uint>& head = mHeads[inThreadIndex];

    while (!mQuit)
    {
        // Wait for jobs
        mSemaphore.Acquire();

        {
            JPH_PROFILE("Executing Jobs");

            // Loop over the queue
            while (head != mTail)
            {
                // Exchange any job pointer we find with a nullptr
                std::atomic<Job*>& job = mQueue[head & (cQueueLength - 1)];
                if (job.load() != nullptr)
                {
                    Job* job_ptr = job.exchange(nullptr);
                    if (job_ptr != nullptr)
                    {
                        // And execute it
                        job_ptr->Execute();
                        job_ptr->Release();
                    }
                }
                head++;
            }
        }
    }

    // Call the thread exit function
    mThreadExitFunction(inThreadIndex);

    JPH_PROFILE_THREAD_END();
}
}; // namespace legs
