#include <gtest/gtest.h>

#include "BigUint.hpp"
#include "Lfsr.hpp"
#include "Statistics.hpp"
#include "Generators.hpp"

#include <array>
#include <cmath>



TEST(BigUintTest, SmallModPowMatchesHandComputation) {

    Modulus mod(BigUint::FromHex("17"));   
    BigUint result = mod.PowMod(BigUint::FromU64(5), BigUint::FromU64(3));

    EXPECT_EQ(result.ToHex(), "A");  
}

TEST(BigUintTest, ModPowWithZeroExponentIsOne) {

    Modulus mod(BigUint::FromHex("FFFF"));
    BigUint result = mod.PowMod(BigUint::FromU64(12345), BigUint::FromU64(0));

    EXPECT_EQ(result.ToHex(), "1");
}

TEST(BigUintTest, MultiplyAndDivModRoundTrip) {

    BigUint a = BigUint::FromHex("123456789ABCDEF0");
    BigUint b = BigUint::FromHex("FEDCBA9876543210");

    BigUint product = a * b;
    auto [q, r] = DivMod(product, b);

    EXPECT_EQ(q.ToHex(), a.ToHex());
    EXPECT_TRUE(r.IsZero());
}

TEST(BigUintTest, ComparisonOperators) {

    BigUint small = BigUint::FromU64(5);
    BigUint big = BigUint::FromU64(500000);

    EXPECT_TRUE(small < big);
    EXPECT_TRUE(big > small);
    EXPECT_TRUE(small == BigUint::FromU64(5));
}




namespace {


    size_t FindPeriod(Lfsr lfsr, size_t maxCheck) {

        std::vector<bool> seq;
        seq.reserve(maxCheck);
        for (size_t i = 0; i < maxCheck; ++i) seq.push_back(lfsr.NextBit());

        for (size_t p = 1; p <= maxCheck / 2; ++p) {
            bool ok = true;
            for (size_t i = 0; i + p < maxCheck; ++i) {
                if (seq[i] != seq[i + p]) { ok = false; break; }
            }
            if (ok) return p;
        }
        return 0;
    }

}

TEST(LfsrTest, L11HasMaximalPeriod) {

    EXPECT_EQ(FindPeriod(Lfsr(11, { 11, 9 }, 1), 5000), 2047u);   
}

TEST(LfsrTest, L9HasMaximalPeriod) {

    EXPECT_EQ(FindPeriod(Lfsr(9, { 9, 8, 6, 5 }, 2), 1200), 511u);  
}

TEST(LfsrTest, L10HasMaximalPeriod) {

    EXPECT_EQ(FindPeriod(Lfsr(10, { 10, 7 }, 3), 2100), 1023u);
}

TEST(LfsrTest, SameSeedGivesSameSequence) {

    Lfsr a(20, { 3, 5, 9, 20 }, 42);
    Lfsr b(20, { 3, 5, 9, 20 }, 42);

    for (int i = 0; i < 1000; ++i) EXPECT_EQ(a.NextBit(), b.NextBit());
}



TEST(StatisticsTest, NormalQuantileMatchesKnownValues) {

    EXPECT_NEAR(NormalQuantile(0.95), 1.644854, 1e-5);
    EXPECT_NEAR(NormalQuantile(0.99), 2.326348, 1e-5);
    EXPECT_NEAR(NormalQuantile(0.5), 0.0, 1e-9);
}

TEST(StatisticsTest, PerfectlyUniformSequencePassesEquiprobability) {

    std::vector<uint8_t> data;
    for (int rep = 0; rep < 100; ++rep)
        for (int v = 0; v < 256; ++v) data.push_back(static_cast<uint8_t>(v));

    auto result = TestEquiprobability(data, 0.05);

    EXPECT_NEAR(result.statistic, 0.0, 1e-9);
    EXPECT_TRUE(result.passed);
}

TEST(StatisticsTest, ConstantSequenceFailsEquiprobability) {

    std::vector<uint8_t> data(100000, 7);

    auto result = TestEquiprobability(data, 0.05);

    EXPECT_FALSE(result.passed);
    EXPECT_GT(result.statistic, result.threshold);
}

TEST(StatisticsTest, PerfectDiagonalCorrelationFailsIndependence) {


    std::vector<uint8_t> data;
    for (int i = 0; i < 200000; ++i) {
        uint8_t k = static_cast<uint8_t>(i % 256);
        data.push_back(k);
        data.push_back(k);
    }

    auto result = TestIndependence(data, 0.05);
    EXPECT_FALSE(result.passed);
}



class AllGeneratorsTest : public ::testing::TestWithParam<GeneratorInfo> {};

TEST_P(AllGeneratorsTest, SameSeedGivesSameBytes) {

    auto a = CreateGenerator(GetParam().id, 555);
    auto b = CreateGenerator(GetParam().id, 555);

    auto bytesA = a->NextBytes(500);
    auto bytesB = b->NextBytes(500);

    EXPECT_EQ(bytesA, bytesB);
}

TEST_P(AllGeneratorsTest, ProducesMoreThanOneDistinctByte) {

    auto gen = CreateGenerator(GetParam().id, 999);
    auto bytes = gen->NextBytes(2000);

    std::array<bool, 256> seen{};
    size_t distinct = 0;
    for (uint8_t b : bytes) if (!std::exchange(seen[b], true)) ++distinct;

    EXPECT_GT(distinct, 1u);
}

INSTANTIATE_TEST_SUITE_P(AllTwelve, AllGeneratorsTest, ::testing::ValuesIn(AllGenerators()),
    [](const testing::TestParamInfo<GeneratorInfo>& info) { return std::string(info.param.code); });


TEST(GeneratorsTest, DifferentSeedsGiveDifferentStreams) {

    auto a = CreateGenerator(GeneratorId::Builtin, 1);
    auto b = CreateGenerator(GeneratorId::Builtin, 2);

    EXPECT_NE(a->NextBytes(64), b->NextBytes(64));
}

TEST(GeneratorsTest, LibrarianFailsEquiprobabilityEarly) {

    auto gen = CreateGenerator(GeneratorId::Librarian, 1);
    auto bytes = gen->NextBytes(2000);

    auto result = TestEquiprobability(bytes, 0.05);
    EXPECT_FALSE(result.passed);
}
