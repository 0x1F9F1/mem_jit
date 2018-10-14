#include <mem/mem_jit_pattern.h>

#include <sljitLir.h>

namespace mem
{
    jit_pattern::jit_pattern(const pattern& pattern)
        : original_length_(pattern.size())
    {
        if (original_length_)
        {
            compile(pattern);
        }
    }

    jit_pattern::~jit_pattern()
    {
        if (scanner_)
        {
            sljit_free_code(scanner_);
        }
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

        struct sljit_compiler* compiler = sljit_create_compiler(NULL);

        const sljit_s32 V_Current = SLJIT_S(0);
        const sljit_s32 V_End     = SLJIT_S(1);
        const sljit_s32 V_Temp    = SLJIT_R(0);

        struct sljit_label* L_ScanLoop = nullptr;

        struct sljit_jump* J_NotFound = nullptr;
        struct sljit_jump** J_Next    = new struct sljit_jump*[size];

        sljit_emit_enter(compiler, 0, SLJIT_ARG1(SW) | SLJIT_ARG2(SW), 1, 2, 0, 0, 0);

        L_ScanLoop = sljit_emit_label(compiler);
        J_NotFound = sljit_emit_cmp(compiler, SLJIT_GREATER, V_Current, 0, V_End, 0);

        for (size_t i = size; i--;)
        {
            const uint8_t byte = bytes[i];
            const uint8_t mask = masks[i];

            if (mask != 0)
            {
                sljit_emit_op1(compiler, SLJIT_MOV_U8 | SLJIT_MEM_LOAD, V_Temp, 0, SLJIT_MEM1(V_Current), i);

                if (mask != 0xFF)
                {
                    sljit_emit_op2(compiler, SLJIT_AND, V_Temp, 0, V_Temp, 0, SLJIT_IMM, mask);
                }

                J_Next[i] = sljit_emit_cmp(compiler, SLJIT_NOT_EQUAL, V_Temp, 0, SLJIT_IMM, byte);
            }
        }

        sljit_emit_return(compiler, SLJIT_MOV, V_Current, 0);

        for (size_t i = size; i--;)
        {
            sljit_set_label(J_Next[i], sljit_emit_label(compiler));
        }

        delete[] J_Next;

        sljit_emit_op2(compiler, SLJIT_ADD, V_Current, 0, V_Current, 0, SLJIT_IMM, 1);

        sljit_set_label(sljit_emit_jump(compiler, SLJIT_JUMP), L_ScanLoop);

        sljit_set_label(J_NotFound, sljit_emit_label(compiler));

        sljit_emit_return(compiler, SLJIT_MOV, SLJIT_IMM, 0);

        scanner_ = (scanner_func) sljit_generate_code(compiler);

        bool success = sljit_get_compiler_error(compiler) == SLJIT_ERR_COMPILED;

        sljit_free_compiler(compiler);

        return success;
    }
}
