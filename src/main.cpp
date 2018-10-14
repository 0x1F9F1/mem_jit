#include <asmjit/asmjit.h>
#include <stdio.h>

#include <mem.h>
#include <mem_pattern.h>

using namespace asmjit;

namespace mem
{
    class jit_pattern
    {
    private:
        using jitted_func = const void*(__fastcall*)(const void* start, const void* end);

        JitRuntime* runtime_ {nullptr};
        jitted_func scanner_ {nullptr};
        size_t original_length_ {0};

        bool compile(const pattern& pattern);

    public:
        jit_pattern(JitRuntime* runtime, const pattern& pattern);
        ~jit_pattern();

        jit_pattern(const jit_pattern&) = delete;
        jit_pattern(jit_pattern&&) = delete;

        template <typename UnaryPredicate>
        pointer scan_predicate(region range, UnaryPredicate pred) const;
    };

    bool jit_pattern::compile(const pattern& pattern)
    {
        CodeHolder code;
        code.init(runtime_->getCodeInfo());

        X86Compiler cc(&code);
        cc.addFunc(FuncSignatureT<const void*, const void*, const void*>(CallConv::kIdHostFastCall));

        X86Gp V_Current = cc.newUIntPtr("Current");
        X86Gp V_End = cc.newUIntPtr("End");

        cc.setArg(0, V_Current);
        cc.setArg(1, V_End);

        Label L_ScanLoop = cc.newLabel();
        Label L_NotFound = cc.newLabel();
        Label L_Next = cc.newLabel();

        cc.bind(L_ScanLoop);
        cc.cmp(V_Current, V_End);
        cc.ja(L_NotFound);

        const std::vector<mem::byte>& bytes = pattern.bytes();
        const std::vector<mem::byte>& masks = pattern.masks();
        const size_t size = pattern.size();

        X86Gp V_Temp = cc.newUInt8("Temp");

        for (size_t i = size; i--;)
        {
            uint8_t byte = bytes.at(i);
            uint8_t mask = !masks.empty() ? masks.at(i) : 0xFF;

            if (mask != 0)
            {
                if (mask == 0xFF)
                {
                    cc.cmp(x86::byte_ptr(V_Current, static_cast<int32_t>(i)), byte);
                }
                else
                {
                    cc.mov(V_Temp, x86::byte_ptr(V_Current, static_cast<int32_t>(i)));
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

        Error err = runtime_->add(&scanner_, &code);

        return !err;
    }

    jit_pattern::jit_pattern(JitRuntime* runtime, const pattern& pattern)
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

    template<typename UnaryPredicate>
    pointer jit_pattern::scan_predicate(region range, UnaryPredicate pred) const
    {
        if (!scanner_)
        {
            return nullptr;
        }

        const void* current = range.start.as<const void*>();
        const void* const end = range.start.add(range.size - original_length_).as<const void*>();

        while ((current = scanner_(current, end)) != nullptr)
        {
            if (pred(current))
            {
                return current;
            }

            // TODO: Is there a better way to do this?
            current = reinterpret_cast<const byte*>(current) + 1;
        }

        return nullptr;
    }
}

int main(int argc, char* argv[])
{
    JitRuntime runtime;

    mem::jit_pattern pattern(&runtime, "01 02 ? 04 05");

    uint8_t data[7] = { 0x50, 0x50, 0x01, 0x02, 0x03, 0x04, 0x05 };

    pattern.scan_predicate({ data, 7 }, [&data] (mem::pointer result)
    {
        printf("%p = %zi\n", result.as<void*>(), result - data);

        return false;
    });

    return 0;
}
