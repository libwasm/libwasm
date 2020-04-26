// DataBuffer.cpp

#include "DataBuffer.h"

#include "common.h"

#include <iostream>

namespace libwasm
{

DataBuffer::DataBuffer()
{
    reset();
}

bool DataBuffer::readFile(std::istream& stream)
{
    stream.seekg(0, std::ios::end);

    size_t fileSize = stream.tellg();

    stream.seekg(0, std::ios::beg);

    container->resize(fileSize);
    stream.read(container->data(), fileSize);
    containerSize = fileSize;

    return (size_t(stream.gcount()) == fileSize);
}

void DataBuffer::reset()
{
    pos = 0;
    containers.clear();
    containers.emplace_back();
    container = &containers.back();
    containerSize = container->size();
}

void DataBuffer::push()
{
    containers.emplace_back();
    container = &containers.back();
    containerSize = container->size();
}

std::string DataBuffer::pop()
{
    auto size = containers.size();

    assert (size > 1);
    std::string result = std::move(containers[size - 1]);

    containers.pop_back();
    container = &containers.back();
    containerSize = container->size();

    return result;
}

void DataBuffer::append(std::string_view str)
{
    container->append(str.data(), str.size());
}

uint64_t DataBuffer::getUleb()
{
    uint64_t result = 0;
    unsigned shift = 0;

    for(;;) {
        uint8_t byte = getU8();

        result |= uint64_t(byte & 0x7f) << shift;

        if ((byte & 0x80) == 0) {
            return result;
        }

        shift += 7;
    }
}

int64_t DataBuffer::getSleb()
{
    int64_t result = 0;
    unsigned shift = 0;

    uint8_t byte;

    do {
        byte = getU8();
        result |= (int64_t(byte & 0x7f) << shift);
        shift += 7;
    } while ((byte & 0x80) != 0);

    if (shift < sizeof(int64_t) * 8 && (byte & 0x40) != 0) {
        result |= (~0LL << shift);
    }

    return result;
}

void DataBuffer::putUleb(uint64_t value)
{
    do {
        uint8_t byte = value & 0x7f;
        value >>= 7;
        if (value != 0) {
            byte |= 0x80;
        }

        putU8(byte);
    } while (value != 0);
}

void DataBuffer::putSleb(int64_t value)
{
    bool more = true;

    while (more) {
        uint8_t byte = value & 0x7f;
        value >>= 7;

        if ((value == 0 && (byte & 0x40) == 0 ) || (value == -1 && (byte & 0x40) != 0)) {
            more = false;
        }  else {
            byte |= 0x80;
        }

        putU8(byte);
    }
}

float DataBuffer::getF()
{
    union
    {
        uint32_t uintValue;
        float result;
    };

    uintValue = getU32();
    return result;
}

void DataBuffer::putF(float value)
{
    union
    {
        uint32_t uintValue;
        float floatValue;
    };

    floatValue = value;
    putU32(uintValue);
}

double DataBuffer::getD()
{
    union
    {
        uint64_t uintValue;
        double result;
    };

    uintValue = getU64();
    return result;
}

void DataBuffer::putD(double value)
{
    union
    {
        uint64_t uintValue;
        double doubleValue;
    };

    doubleValue = value;
    putU64(uintValue);
}

void DataBuffer::dump(std::ostream& os, size_t startOffset, size_t endOffset)
{
    dumpChars(os, { &containers.back()[startOffset], endOffset - startOffset }, startOffset);
}

};
