#pragma once

#include <sys/resource.h>
#include <sys/time.h>

namespace legs
{
class Memory
{
  public:
    // Get resident memory usage in KB.
    static long int GetUsage()
    {
        rusage ru = {};
        if (getrusage(RUSAGE_SELF, &ru) == 0)
        {
            return ru.ru_maxrss;
        }

        return -1;
    }
};
}; // namespace legs
