// DataBuffer.h

#ifndef DATABUFFER_H
#define DATABUFFER_H

#include "common.h"

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

namespace libwasm
{
class DataBuffer
{
    public:
        DataBuffer();

        bool readFile(std::istream& stream);

        size_t getPos() const
        {
            return pointer - container->data();
        }

        const char* getPointer() const
        {
            return pointer;
        }

        void setPos(size_t p)
        {
            pointer = container->data() + p;
        }

        auto size() const
        {
            return container->size();
        }

        void clear()
        {
            container->clear();
            pointer = nullptr;
            endPointer = nullptr;
        }

        void reset();

        bool atEnd() const
        {
            return pointer == endPointer;
        }

        char* data()
        {
            return container->data();
        }

        char nextChar()
        {
            return getU8();
        }

        // since thw container is a string and is null terminated, we don't
        // need to check for overflow.
        char peekChar() const
        {
            return *pointer;
        }

        char bumpPeekChar()
        {
            assert(!atEnd());
            return *(++pointer);
        }

        void skipChars(char c) {
            assert(c != 0);
            while (*pointer == c) {
                ++pointer;
            }
        }

        char peekChar(int n) const
        {
            if (n < 0) {
                if (-n > pointer - container->data()) {
                    return '\0';
                }
            } else if (size_t(n) >= size_t(endPointer - pointer)) {
                return '\0';
            }

            return *(pointer + n);
        }

        bool peekChars(std::string_view chars) const
        {
            if (chars.size() >= size_t(endPointer - pointer)) {
                return false;
            }

            return std::string_view(getPointer(), chars.size()) == chars;
        }

        void bump(int count = 1)
        {
            assert (count <= (endPointer - pointer));
            pointer += count;
        }

        uint8_t getU8()
        {
            assert(!atEnd());
            return *(pointer++);
        }

        void putU8(uint8_t value)
        {
            container->push_back(value);
        }

        int8_t getI8()
        {
            assert(!atEnd());
            return *(pointer++);
        }

        void putI8(int8_t value)
        {
            container->push_back(value);
        }

        uint16_t getU16()
        {
            return uint16_t(uint16_t(getU8()) | uint16_t(getU8()) << 8);
        }

        uint32_t getU32()
        {
            return uint32_t(uint32_t(getU16()) | uint32_t(getU16()) << 16);
        }

        void putU32(uint32_t value)
        {
            putU8(uint8_t(value));
            putU8(uint8_t(value >> 8));
            putU8(uint8_t(value >> 16));
            putU8(uint8_t(value >> 24));
        }

        uint64_t getU64()
        {
            return uint64_t(uint64_t(getU32()) | uint64_t(getU32()) << 32);
        }

        void putU64(uint64_t value)
        {
            putU8(uint8_t(value));
            putU8(uint8_t(value >> 8));
            putU8(uint8_t(value >> 16));
            putU8(uint8_t(value >> 24));
            putU8(uint8_t(value >> 32));
            putU8(uint8_t(value >> 40));
            putU8(uint8_t(value >> 48));
            putU8(uint8_t(value >> 56));
        }

        int32_t getI32()
        {
            return int32_t(getU32());
        }

        void putI32(uint32_t value)
        {
            putU32(value);
        }

        int64_t getI64()
        {
            return int64_t(getU64());
        }

        void putI64(uint32_t value)
        {
            putU64(value);
        }

        v128_t getV128()
        {
            v128_t result;

            for (int i = 0; i < 16; ++i) {
                result[i] = getU8();
            }

            return result;
        }

        void putV128(const v128_t& value)
        {
            for (int i = 0; i < 16; ++i) {
                putU8(value[i]);
            }
        }

        float getF();
        void putF(float value);
        double getD();
        void putD(double value);

        uint64_t getUleb();
        void putUleb(uint64_t value);
        int64_t getSleb();
        void putSleb(int64_t value);

        uint32_t getU32leb()
        {
            return uint32_t(getUleb());
        }

        void putU32leb(uint32_t value)
        {
            putUleb(value);
        }

        int32_t getI32leb()
        {
            return int32_t(getSleb());
        }

        void putI32leb(int32_t value)
        {
            putSleb(value);
        }

        uint64_t getU64leb()
        {
            return uint64_t(getUleb());
        }

        void putU64leb(uint64_t value)
        {
            putUleb(value);
        }

        int64_t getI64leb()
        {
            return int64_t(getSleb());
        }

        void putI64leb(int64_t value)
        {
            putSleb(value);
        }

        void push();
        std::string pop();
        void append(std::string_view str);

    private:
        char* pointer = nullptr;
        char* endPointer = nullptr;
        std::vector<std::string> containers;
        std::string *container;
};
};

#endif
