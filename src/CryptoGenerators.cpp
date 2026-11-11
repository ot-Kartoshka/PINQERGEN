#include "CryptoGenerators.hpp"
#include "BigUint.hpp"

#include <random>


namespace {

    const BigUint kBmP = BigUint::FromHex("CEA42B987C44FA642D80AD9F51F10457690DEF10C83D0BC1BCEE12FC3B6093E3");
    const BigUint kBmA = BigUint::FromHex("5B88C41246790891C095E2878880342E88C79974303BD0400B090FE38A688356");

    const BigUint kBbsP = BigUint::FromHex("D5BBB96D30086EC484EBA3D7F9CAEB07");
    const BigUint kBbsQ = BigUint::FromHex("425D2B9BFDB25B9CF6C416CC6E37B59C1F");


    BigUint RandomBelow(const BigUint& bound, std::mt19937_64& gen) {

        size_t limbCount = bound.limbs.size();

        BigUint candidate;
        candidate.limbs.resize(limbCount);
        for (size_t i = 0; i < limbCount; ++i) candidate.limbs[i] = gen();

        return DivMod(candidate, bound).second;
    }


    class BlumMicaliGenerator : public ByteGenerator {

    public:

        BlumMicaliGenerator(uint64_t seed, bool byteMode) : mod(kBmP), byteMode(byteMode) {

            std::mt19937_64 gen(seed);
            state = RandomBelow(kBmP, gen);

            BigUint one = BigUint::FromU64(1);
            half = DivMod(kBmP - one, BigUint::FromU64(2)).first;   // (p - 1) / 2
        }

        uint8_t NextByte() override {

            if (byteMode) return NextStateByte();

            uint8_t byte = 0;
            for (int bit = 0; bit < 8; ++bit) byte = static_cast<uint8_t>((byte << 1) | (NextStateBit() ? 1 : 0));
            return byte;
        }

    private:

        bool NextStateBit() {

            bool bit = state < half;
            state = mod.PowMod(kBmA, state);
            return bit;
        }

        uint8_t NextStateByte() {

            BigUint scaled = state * BigUint::FromU64(256);
            BigUint quotient = DivMod(scaled, kBmP).first;
            uint8_t k = quotient.limbs.empty() ? 0 : static_cast<uint8_t>(quotient.limbs[0]);

            state = mod.PowMod(kBmA, state);
            return k;
        }

        Modulus mod;
        BigUint state;
        BigUint half;
        bool byteMode;
    };


    class BbsGenerator : public ByteGenerator {

    public:

        BbsGenerator(uint64_t seed, bool byteMode) : mod(kBbsP* kBbsQ), byteMode(byteMode) {

            std::mt19937_64 gen(seed);

            do {
                state = RandomBelow(mod.Value(), gen);
            } while (state.limbs.empty() || (state.limbs.size() == 1 && state.limbs[0] < 2));
        }

        uint8_t NextByte() override {

            if (byteMode) {
                state = mod.MulMod(state, state);
                return state.limbs.empty() ? 0 : static_cast<uint8_t>(state.limbs[0] & 0xFF);
            }

            uint8_t byte = 0;
            for (int bit = 0; bit < 8; ++bit) {
                state = mod.MulMod(state, state);
                bool b = state.limbs.empty() ? false : (state.limbs[0] & 1ULL);
                byte = static_cast<uint8_t>((byte << 1) | (b ? 1 : 0));
            }
            return byte;
        }

    private:

        Modulus mod;
        BigUint state;
        bool byteMode;
    };

}


std::unique_ptr<ByteGenerator> MakeBlumMicali(uint64_t seed, bool byteMode) {

    return std::make_unique<BlumMicaliGenerator>(seed, byteMode);
}

std::unique_ptr<ByteGenerator> MakeBbs(uint64_t seed, bool byteMode) {

    return std::make_unique<BbsGenerator>(seed, byteMode);
}
