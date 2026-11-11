#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <compare>


class BigUint {

public:

    BigUint() = default;

    static BigUint FromHex(std::string_view hex);
    static BigUint FromU64(uint64_t value);

    std::string ToHex() const;

    bool IsZero() const;
    size_t BitLength() const;
    bool GetBit(size_t index) const;

    BigUint operator+(const BigUint& other) const;
    BigUint operator-(const BigUint& other) const;
    BigUint operator*(const BigUint& other) const;

    BigUint ShiftLimbs(size_t k) const;
    BigUint LowLimbs(size_t k) const;

    std::strong_ordering operator<=>(const BigUint& other) const;
    bool operator==(const BigUint& other) const;

    std::vector<uint64_t> limbs;

    void Trim();
};

std::pair<BigUint, BigUint> DivMod(const BigUint& a, const BigUint& b);


class Modulus {

public:

    explicit Modulus(BigUint n);

    const BigUint& Value() const { return n; }

    BigUint Reduce(const BigUint& x) const;
    BigUint MulMod(const BigUint& a, const BigUint& b) const;
    BigUint PowMod(const BigUint& base, const BigUint& exp) const;

private:

    BigUint n;
    BigUint mu;
    size_t k;
};
