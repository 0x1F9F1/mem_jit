#include <cstdio>
#include "mem_jit_pattern.h"

#include <asmjit/asmjit.h>

int main(int argc, char* argv[])
{
    asmjit::JitRuntime runtime;

    mem::pattern_settings settings {1,1};

    mem::pattern pattern("01 02 ? 04 05", settings);
    mem::jit_pattern jit_pattern(&runtime, pattern);

    uint8_t data[7] = { 0x50, 0x50, 0x01, 0x02, 0x03, 0x04, 0x05 };

    jit_pattern.scan_predicate({ data, 7 }, [&data] (mem::pointer result)
    {
        printf("%p = %zi\n", result.as<void*>(), result - data);

        return false;
    });

    return 0;
}
