#include "Generators.hpp"
#include "CryptoGenerators.hpp"
#include "Lfsr.hpp"

#include <bit>
#include <fstream>
#include <random>
#include <sstream>


namespace {


    constexpr std::string_view kDefaultLibrarianText = 
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed do eiusmod tempor incididunt ut "
        "labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris "
        "nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit "
        "esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in "
        "culpa qui officia deserunt mollit anim id est laborum.\n\n"
        "Curabitur pretium tincidunt lacus. Nulla gravida orci a odio. Nullam varius, turpis et commodo "
        "pharetra, est eros bibendum elit, nec luctus magna felis sollicitudin mauris. Integer in mauris eu "
        "nibh euismod gravida. Duis ac tellus et risus vulputate vehicula. Donec lobortis risus a elit. "
        "Etiam tempor. Ut ullamcorper, ligula eu tempor congue, eros est euismod turpis, id tincidunt sapien "
        "risus a quam. Maecenas fermentum consequat mi. Donec fermentum. Pellentesque habitant morbi tristique "
        "senectus et netus et malesuada fames ac turpis egestas.\n\n"
        "Vestibulum nonummy aliquem ante. Lorem ipsum dolor sit amet, consectetuer adipiscing elit. Mauris "
        "accumsan nulla vel diam. Sed in lacus ut enim adipiscing aliquet. Nulla venenatis. In pede mi, aliquet "
        "sit amet, euismod in, auctor ut, ligula. Aliquam dapibus tincidunt metus. Praesent justo dolor, "
        "lobortis quis, lobortis dignissim, pulvinar ac, lorem. Vestibulum sed ante. Donec sagittis euismod "
        "purus. Sed ut perspiciatis unde omnis iste natus error sit voluptatem accusantium doloremque "
        "laudantium, totam rem aperiam, eaque ipsa quae ab illo inventore veritatis et quasi architecto "
        "beatae vitae dicta sunt explicabo. Nemo enim ipsam voluptatem quia voluptas sit aspernatur aut "
        "odit aut fugit, sed quia consequuntur magni dolores eos qui ratione voluptatem sequi nesciunt. "
        "Neque porro quisquam est, qui dolorem ipsum quia dolor sit amet, consectetur, adipisci velit, sed "
        "quia non numquam eius modi tempora incidunt ut labore et dolore magnam aliquam quaerat voluptatem. "
        "Ut enim ad minima veniam, quis nostrum exercitationem ullam corporis suscipit laboriosam, nisi ut "
        "aliquid ex ea commodi consequatur? Quis autem vel eum iure reprehenderit qui in ea voluptate velit "
        "esse quam nihil molestiae.";


    uint64_t MixSeed(uint64_t seed, GeneratorId id) {

        uint64_t x = seed + static_cast<uint64_t>(id) * 0x9E3779B97F4A7C15ULL;
        x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
        x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
        return x ^ (x >> 31);
    }

    class BuiltinGenerator : public ByteGenerator {

    public:

        explicit BuiltinGenerator(uint64_t seed) : engine(static_cast<std::minstd_rand::result_type>(seed)) {}

        uint8_t NextByte() override { return static_cast<uint8_t>(engine() & 0xFF); }

    private:

        std::minstd_rand engine;
    };


    class LehmerGenerator : public ByteGenerator {

    public:

        LehmerGenerator(uint64_t seed, bool takeHighByte) : takeHighByte(takeHighByte) {

            std::mt19937_64 gen(seed);
            do { x = static_cast<uint32_t>(gen()); } while (x == 0);
        }

        uint8_t NextByte() override {

            x = kMul * x + kInc;
            return takeHighByte ? static_cast<uint8_t>(x >> 24) : static_cast<uint8_t>(x & 0xFF);
        }

    private:

        static constexpr uint32_t kMul = (1u << 16) + 1u;
        static constexpr uint32_t kInc = 119u;

        uint32_t x;
        bool takeHighByte;
    };


    class LfsrByteGenerator : public ByteGenerator {

    public:

        LfsrByteGenerator(size_t length, std::initializer_list<size_t> taps, uint64_t seed) : lfsr(length, taps, seed) {}

        uint8_t NextByte() override {

            uint8_t byte = 0;
            for (int i = 0; i < 8; ++i) byte = static_cast<uint8_t>((byte << 1) | (lfsr.NextBit() ? 1 : 0));
            return byte;
        }

    private:

        Lfsr lfsr;
    };


    class GeffeGenerator : public ByteGenerator {

    public:

        explicit GeffeGenerator(uint64_t seed) : x(11, { 11, 9 }, seed), y(9, { 9, 8, 6, 5 }, seed + 1), s(10, { 10, 7 }, seed + 2) {}

        uint8_t NextByte() override {

            uint8_t byte = 0;
            for (int i = 0; i < 8; ++i) byte = static_cast<uint8_t>((byte << 1) | (NextGeffeBit() ? 1 : 0));
            return byte;
        }

    private:

        bool NextGeffeBit() {

            bool bx = x.NextBit();
            bool by = y.NextBit();
            bool bs = s.NextBit();

            return bs ? bx : by;
        }

        Lfsr x, y, s;
    };


    class LibrarianGenerator : public ByteGenerator {

    public:

        explicit LibrarianGenerator(std::string_view textPath) {

            if (!textPath.empty()) {

                std::ifstream file(std::string(textPath), std::ios::binary);
                if (file) {
                    std::ostringstream ss;
                    ss << file.rdbuf();
                    text = ss.str();
                }
            }

            if (text.empty()) text = std::string(kDefaultLibrarianText);
        }

        uint8_t NextByte() override {

            uint8_t byte = static_cast<uint8_t>(text[position]);
            position = (position + 1) % text.size();
            return byte;
        }

    private:

        std::string text;
        size_t position = 0;
    };


    class WolframGenerator : public ByteGenerator {

    public:

        explicit WolframGenerator(uint64_t seed) {

            std::mt19937_64 gen(seed);
            do { r = static_cast<uint32_t>(gen()); } while (r == 0);
        }

        uint8_t NextByte() override {

            uint8_t byte = 0;
            for (int i = 0; i < 8; ++i) byte = static_cast<uint8_t>((byte << 1) | (NextStateBit() ? 1 : 0));
            return byte;
        }

    private:

        bool NextStateBit() {

            bool bit = r & 1u;
            r = std::rotl(r, 1) ^ (r | std::rotr(r, 1));
            return bit;
        }

        uint32_t r;
    };

}


const std::vector<GeneratorInfo>& AllGenerators() {

    static const std::vector<GeneratorInfo> table{
        { GeneratorId::Builtin,         "builtin",   "Вбудований генератор (LCG, std::minstd_rand)", true },
        { GeneratorId::LehmerLow,       "lehmer_lo", "Генератор Лемера -- LehmerLow",                true },
        { GeneratorId::LehmerHigh,      "lehmer_hi", "Генератор Лемера -- LehmerHigh",                true },
        { GeneratorId::L20,             "l20",       "L20 (M-послідовність, період 2^20-1)",         false },
        { GeneratorId::L89,             "l89",       "L89 (M-послідовність, період 2^89-1)",         false },
        { GeneratorId::Geffe,           "geffe",     "Генератор Джиффі (L11, L9, L10)",               false },
        { GeneratorId::Librarian,       "librarian", "Генератор \"Бібліотекар\"",                     true },
        { GeneratorId::Wolfram,         "wolfram",   "Генератор Вольфрама",                           false },
        { GeneratorId::BlumMicali,      "bm",        "Генератор Блюма-Мікалі (BM)",                  false },
        { GeneratorId::BlumMicaliBytes, "bm_bytes",  "Блюм-Мікалі, байтова модифікація",              true },
        { GeneratorId::Bbs,             "bbs",       "Генератор Блюма-Блюма-Шуба (BBS)",              false },
        { GeneratorId::BbsBytes,        "bbs_bytes", "Блюм-Блюма-Шуба, байтова модифікація",          true },
    };

    return table;
}

std::unique_ptr<ByteGenerator> CreateGenerator(GeneratorId id, uint64_t seed, std::string_view textSourcePath) {

    uint64_t mixed = MixSeed(seed, id);

    switch (id) {

    case GeneratorId::Builtin:         return std::make_unique<BuiltinGenerator>(mixed);
    case GeneratorId::LehmerLow:       return std::make_unique<LehmerGenerator>(mixed, false);
    case GeneratorId::LehmerHigh:      return std::make_unique<LehmerGenerator>(mixed, true);
    case GeneratorId::L20:             return std::make_unique<LfsrByteGenerator>(20, std::initializer_list<size_t>{ 3, 5, 9, 20 }, mixed);
    case GeneratorId::L89:             return std::make_unique<LfsrByteGenerator>(89, std::initializer_list<size_t>{ 38, 89 }, mixed);
    case GeneratorId::Geffe:           return std::make_unique<GeffeGenerator>(mixed);
    case GeneratorId::Librarian:       return std::make_unique<LibrarianGenerator>(textSourcePath);
    case GeneratorId::Wolfram:         return std::make_unique<WolframGenerator>(mixed);
    case GeneratorId::BlumMicali:      return MakeBlumMicali(mixed, false);
    case GeneratorId::BlumMicaliBytes: return MakeBlumMicali(mixed, true);
    case GeneratorId::Bbs:             return MakeBbs(mixed, false);
    case GeneratorId::BbsBytes:        return MakeBbs(mixed, true);
    }

    return nullptr;
}
