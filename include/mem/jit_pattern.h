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

#if !defined(MEM_JIT_PATTERN_BRICK_H)
#define MEM_JIT_PATTERN_BRICK_H

#include <mem_pattern.h>

namespace mem
{
    using scanner_func = const void*(*)(const void* start, const void* end);

    class jit_runtime
    {
    private:
        void* context_ {nullptr};

    public:
        jit_runtime();
        ~jit_runtime();

        jit_runtime(const jit_runtime&) = delete;
        jit_runtime(jit_runtime&&) = delete;

        scanner_func compile(const pattern& pattern);
        void release(scanner_func scanner);
    };

    class jit_pattern
    {
    private:
        jit_runtime* runtime_ {nullptr};
        scanner_func scanner_ {nullptr};
        size_t original_length_ {0};

    public:
        jit_pattern(jit_runtime* runtime, const pattern& pattern);
        ~jit_pattern();

        jit_pattern(const jit_pattern&) = delete;
        jit_pattern(jit_pattern&&) = delete;

        template <typename UnaryPredicate>
        pointer scan_predicate(region range, UnaryPredicate pred) const;

        std::vector<pointer> scan_all(region range) const;
    };

    template<typename UnaryPredicate>
    pointer inline jit_pattern::scan_predicate(region range, UnaryPredicate pred) const
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

    inline std::vector<pointer> jit_pattern::scan_all(region range) const
    {
        std::vector<pointer> results;

        scan_predicate(range, [&results] (pointer result)
        {
            results.emplace_back(result);

            return false;
        });

        return results;
    }
}

#endif // MEM_JIT_PATTERN_BRICK_H
