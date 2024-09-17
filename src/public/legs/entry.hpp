#pragma once

#include <cstring>
#include <exception>

#include <legs/engine.hpp>
#include <legs/log.hpp>

namespace legs
{
extern std::shared_ptr<Engine> g_engine;

static bool HasLaunchArg(const char* name, const char* value, int argc, char** argv)
{
    for (int i = 0; i < argc; i++)
    {
        if (std::strcmp(name, argv[i]) == 0)
        {
            if (value == nullptr)
            {
                return true;
            }
            if (i < argc - 1 && std::strcmp(value, argv[i + 1]) == 0)
            {
                return true;
            }
        }
    }
    return false;
}

static int LEGS_Init(int /*argc*/, char** /*argv*/)
{
    try
    {
        g_engine = std::make_shared<legs::Engine>();
        return 0;
    }
    catch (std::exception& ex)
    {
        LOG_FATAL("Unhandled exception: {}", ex.what());
        legs::Log::Flush();

        if (g_engine != nullptr)
        {
            g_engine.reset();
        }

        return -1;
    }
}

static int LEGS_Run()
{
    try
    {
        auto code = g_engine->Run();
        g_engine.reset();
        return code;
    }
    catch (std::exception& ex)
    {
        LOG_FATAL("Unhandled exception: {}", ex.what());
        legs::Log::Flush();

        if (g_engine != nullptr)
        {
            g_engine.reset();
        }

        return -1;
    }
}
}; // namespace legs
