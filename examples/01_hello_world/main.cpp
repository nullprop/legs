#include <legs/entry.hpp>
#include <legs/log.hpp>

using namespace legs;

int main(int argc, char** argv)
{
    Log::SetLogLevel(LogLevel::Debug);

    auto code = LEGS_Init(argc, argv);
    if (code < 0)
    {
        return code;
    }

    g_engine->GetWindow()->SetTitle("01_hello_world");

    return LEGS_Run();
}
