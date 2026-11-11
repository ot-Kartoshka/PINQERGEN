#pragma once

#include <cstdint>
#include <vector>
#include <initializer_list>


class Lfsr {

public:

    Lfsr(size_t length, std::initializer_list<size_t> taps, uint64_t seed);

    bool NextBit();

private:

    std::vector<bool> history;
    std::vector<size_t> taps;
    size_t length;
};
