#include "mem_jit_pattern.h"

#include <asmjit/asmjit.h>

namespace mem
{
    jit_pattern::jit_pattern(asmjit::JitRuntime* runtime, const pattern& pattern)
        : runtime_(runtime)
        , original_length_(pattern.size())
    {
        if (original_length_)
        {
            compile(pattern);
        }
    }

    jit_pattern::~jit_pattern()
    {
        runtime_->release(scanner_);
    }

    bool jit_pattern::compile(const pattern& pattern)
    {
        const size_t size = pattern.trimmed_size();

        if (!size)
        {
            return false;
        }

        const byte* bytes = pattern.bytes();
        const byte* masks = pattern.masks();

        asmjit::CodeHolder code;
        code.init(runtime_->getCodeInfo());

        asmjit::X86Compiler cc(&code);
        cc.addFunc(asmjit::FuncSignatureT<const void*, const void*, const void*>());

        asmjit::X86Gp V_Current = cc.newUIntPtr("Current");
        asmjit::X86Gp V_End     = cc.newUIntPtr("End");
        asmjit::X86Gp V_Temp    = cc.newUInt8("Temp");

        asmjit::Label L_ScanLoop = cc.newLabel();
        asmjit::Label L_NotFound = cc.newLabel();
        asmjit::Label L_Next     = cc.newLabel();

        cc.setArg(0, V_Current);
        cc.setArg(1, V_End);

        cc.bind(L_ScanLoop);
        cc.cmp(V_Current, V_End);
        cc.ja(L_NotFound);

        for (size_t i = size; i--;)
        {
            const uint8_t byte = bytes[i];
            const uint8_t mask = masks[i];

            if (mask != 0)
            {
                if (mask == 0xFF)
                {
                    cc.cmp(asmjit::x86::byte_ptr(V_Current, static_cast<int32_t>(i)), byte);
                }
                else
                {
                    cc.mov(V_Temp, asmjit::x86::byte_ptr(V_Current, static_cast<int32_t>(i)));
                    cc.and_(V_Temp, mask);
                    cc.cmp(V_Temp, byte);
                }

                cc.jne(L_Next);
            }
        }

        cc.ret(V_Current);

        cc.bind(L_Next);
        cc.inc(V_Current);
        cc.jmp(L_ScanLoop);

        cc.bind(L_NotFound);
        cc.xor_(V_Current, V_Current);
        cc.ret(V_Current);

        cc.endFunc();
        cc.finalize();

        asmjit::Error err = runtime_->add(&scanner_, &code);

        return !err;
    }
}
