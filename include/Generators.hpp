#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>


class ByteGenerator {

public:

    virtual ~ByteGenerator() = default;
    virtual uint8_t NextByte() = 0;

    std::vector<uint8_t> NextBytes(size_t count) {

        std::vector<uint8_t> out(count);
        for (uint8_t& b : out) b = NextByte();
        return out;
    }
};


enum class GeneratorId {
    Builtin,
    LehmerLow,
    LehmerHigh,
    L20,
    L89,
    Geffe,
    Librarian,
    Wolfram,
    BlumMicali,
    BlumMicaliBytes,
    Bbs,
    BbsBytes
};

struct GeneratorInfo {
    GeneratorId id;
    std::string_view code;
    std::string_view title;
    bool isNative8Bit;
};

const std::vector<GeneratorInfo>& AllGenerators();

std::unique_ptr<ByteGenerator> CreateGenerator(GeneratorId id, uint64_t seed, std::string_view textSourcePath = {});
