/*
    Copyright 2018 Brick

    Permission is hereby granted, free of charge, to any person obtaining a copy of this software
    and associated documentation files (the "Software"), to deal in the Software without restriction,
    including without limitation the rights to use, copy, modify, merge, publish, distribute,
    sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all copies or
    substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
    BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
    DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "mem/jit_scanner.h"

#include <asmjit/asmjit.h>

namespace mem
{
    class jit_runtime::impl
    {
    private:
        asmjit::JitRuntime runtime_;

    public:
        scanner_func compile(const pattern& pattern)
        {
            if (!pattern)
            {
                return nullptr;
            }

            const size_t trimmed_size = pattern.trimmed_size();

            if (!trimmed_size)
            {
                return nullptr;
            }

            const byte* bytes = pattern.bytes();
            const byte* masks = pattern.masks();

            using namespace asmjit;

            CodeHolder code;
            code.init(runtime_.getCodeInfo());

            X86Compiler cc(&code);
            cc.addFunc(FuncSignatureT<const void*, const void*, const void*>());

            X86Gp V_Current   = cc.newUIntPtr("Current");
            X86Gp V_End       = cc.newUIntPtr("End");
            X86Gp V_Temp      = cc.newUIntPtr("Temp");
            X86Xmm V_ScanMask = cc.newXmmReg(TypeId::kU8x16);
            X86Xmm V_TempMask = cc.newXmmReg(TypeId::kU8x16);

            X86Gp V_Temp8   = V_Temp.r8();
            X86Gp V_Temp32  = V_Temp.r32();

            Label L_ScanLoop  = cc.newLabel();
            Label L_NotFound  = cc.newLabel();
            Label L_Next      = cc.newLabel();
            Label L_FindMain  = cc.newLabel();
            Label L_FindTail  = cc.newLabel();
            Label L_FindTail2 = cc.newLabel();
            Label L_Found = cc.newLabel();

            cc.setArg(0, V_Current);
            cc.setArg(1, V_End);

            const size_t original_size = pattern.size();
            const size_t skip_pos = pattern.get_skip_pos();

            if (skip_pos != SIZE_MAX)
            {
                cc.movdqa(V_ScanMask, cc.newXmmConst(ConstScope::kConstScopeLocal, Data128::fromU8(bytes[skip_pos])));
            }

            cc.sub(V_End, original_size);

            cc.cmp(V_Current, V_End);
            cc.ja(L_NotFound);
            cc.jmp(L_ScanLoop);

            if (skip_pos != SIZE_MAX)
            {
            cc.bind(L_FindMain);
                cc.add(V_Current, 16);

            cc.bind(L_Next);
                cc.add(V_Current, 1);
                cc.lea(V_Temp, x86::ptr(V_Current, 16));
                cc.cmp(V_Temp, V_End);
                cc.ja(L_FindTail);

                cc.lddqu(V_TempMask, x86::ptr_128(V_Current, int32_t(skip_pos)));
                cc.pcmpeqb(V_TempMask, V_ScanMask);
                cc.pmovmskb(V_Temp32, V_TempMask);

                cc.test(V_Temp32, V_Temp32);
                cc.jz(L_FindMain);

                cc.bsf(V_Temp32, V_Temp32);
                cc.add(V_Current, V_Temp);
                cc.jmp(L_ScanLoop);

            cc.bind(L_FindTail2);
                cc.add(V_Current, 1);

            cc.bind(L_FindTail);
                cc.cmp(V_Current, V_End);
                cc.ja(L_NotFound);

                cc.cmp(x86::byte_ptr(V_Current, int32_t(skip_pos)), bytes[skip_pos]);
                cc.jnz(L_FindTail2);
            }
            else
            {
            cc.bind(L_Next);
                cc.add(V_Current, 1);
                cc.cmp(V_Current, V_End);
                cc.ja(L_NotFound);
            }

        cc.bind(L_ScanLoop);
            for (size_t i = trimmed_size; i--;)
            {
                const uint8_t byte = bytes[i];
                const uint8_t mask = masks[i];

                if (mask != 0)
                {
                    if (mask == 0xFF)
                    {
                        cc.cmp(x86::byte_ptr(V_Current, int32_t(i)), byte);
                    }
                    else
                    {
                        cc.mov(V_Temp8, x86::byte_ptr(V_Current, int32_t(i)));
                        cc.and_(V_Temp8, mask);
                        cc.cmp(V_Temp8, byte);
                    }

                    cc.jne(L_Next);
                }
            }

            cc.jmp(L_Found);

        cc.bind(L_NotFound);
            cc.xor_(V_Current, V_Current);

        cc.bind(L_Found);
            cc.ret(V_Current);

            cc.endFunc();
            cc.finalize();

            scanner_func result = nullptr;

            Error err = runtime_.add(&result, &code);

            if (err && result)
            {
                release(result);

                result = nullptr;
            }

            return result;
        }

        void release(scanner_func scanner)
        {
            if (scanner)
            {
                runtime_.release(scanner);
            }
        }
    };

    jit_runtime::jit_runtime()
        : impl_(new impl())
    { }

    jit_runtime::~jit_runtime() = default;

    jit_scanner::jit_scanner(jit_runtime* runtime, const pattern& pattern)
        : runtime_(runtime)
        , scanner_(runtime_->compile(pattern))
    { }

    jit_scanner::~jit_scanner()
    {
        if (runtime_ && scanner_)
        {
            runtime_->release(scanner_);
        }
    }

    jit_scanner::jit_scanner(jit_scanner&& rhs)
    {
        runtime_ = rhs.runtime_;
        scanner_ = rhs.scanner_;

        rhs.runtime_ = nullptr;
        rhs.scanner_ = nullptr;
    }

    scanner_func jit_runtime::compile(const pattern& pattern)
    {
        return impl_->compile(pattern);
    }

    void jit_runtime::release(scanner_func scanner)
    {
        return impl_->release(scanner);
    }
}
