#include <cstdio>
#include "mem_jit_pattern.h"

#include <asmjit/asmjit.h>

int main(int argc, char* argv[])
{
    asmjit::JitRuntime runtime;

    mem::jit_pattern pattern(&runtime, "01 02 ? 04 05");

    uint8_t data[7] = { 0x50, 0x50, 0x01, 0x02, 0x03, 0x04, 0x05 };

    pattern.scan_predicate({ data, 7 }, [&data] (mem::pointer result)
    {
        printf("%p = %zi\n", result.as<void*>(), result - data);

        return false;
    });

    return 0;
}
