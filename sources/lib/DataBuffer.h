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
            return pos;
        }

        const char* getPointer() const
        {
            return container->data() + pos;
        }

        void setPos(size_t p)
        {
            pos = p;
        }

        size_t size() const
        {
            return container->size();
        }

        void clear()
        {
            container->clear();
            pos = 0;
            containerSize = 0;
        }

        void reset();

        bool atEnd() const
        {
            return pos == containerSize;
        }

        void resize(size_t newSize)
        {
            container->resize(newSize);
        }

        char* data()
        {
            return container->data();
        }

        char nextChar()
        {
            return getU8();
        }

        char peekChar(size_t n = 0) const
        {
            if (pos + n >= containerSize) {
                return '\0';
            }

            return (*container)[pos + n];
        }

        bool peekChars(std::string_view chars) const
        {
            if (pos + chars.size() >= containerSize) {
                return false;
            }

            return std::string_view(getPointer(), chars.size()) == chars;
        }

        void bump(int count = 1)
        {
            assert(pos + count <= containerSize);
            pos += count;
        }

        uint8_t getU8()
        {
            assert(!atEnd());
            return  (*container)[pos++];
        }

        void putU8(uint8_t value)
        {
            container->push_back(value);
        }

        int8_t getI8()
        {
            assert(!atEnd());
            return  (*container)[pos++];
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

        void dump(std::ostream& os, size_t startOffset, size_t endOffset);

        void push();
        std::string pop();
        void append(std::string_view str);

    private:
        size_t pos = 0;
        size_t containerSize = 0;
        std::vector<std::string> containers;
        std::string *container;
};
};

#endif
