#include "Lfsr.hpp"

#include <random>


Lfsr::Lfsr(size_t length, std::initializer_list<size_t> taps, uint64_t seed) : taps(taps), length(length) {

    history.resize(length);

    std::mt19937_64 gen(seed);
    std::bernoulli_distribution coin(0.5);

    bool allZero = true;
    for (size_t i = 0; i < length; ++i) {
        history[i] = coin(gen);
        if (history[i]) allZero = false;
    }

    if (allZero) history[length - 1] = true;
}

bool Lfsr::NextBit() {

    bool feedback = false;
    for (size_t tap : taps) {
        feedback = feedback ^ history[length - tap];
    }

    history.erase(history.begin());
    history.push_back(feedback);

    return feedback;
}
