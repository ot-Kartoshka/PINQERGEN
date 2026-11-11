#include "BigUint.hpp"

#include <algorithm>
#include <cctype>


void BigUint::Trim() {

    while (!limbs.empty() && limbs.back() == 0) limbs.pop_back();
}

BigUint BigUint::FromU64(uint64_t value) {

    BigUint r;
    if (value != 0) r.limbs.push_back(value);
    return r;
}

BigUint BigUint::FromHex(std::string_view hex) {

    if (hex.starts_with("0x") || hex.starts_with("0X")) hex.remove_prefix(2);

    BigUint r;
    r.limbs.assign((hex.size() + 15) / 16, 0);

    size_t pos = hex.size();
    for (size_t limb = 0; pos > 0; ++limb) {

        size_t chunkLen = std::min<size_t>(16, pos);
        std::string_view chunk = hex.substr(pos - chunkLen, chunkLen);

        uint64_t value = 0;
        for (char c : chunk) {
            value <<= 4;
            if (c >= '0' && c <= '9') value |= static_cast<uint64_t>(c - '0');
            else if (c >= 'a' && c <= 'f') value |= static_cast<uint64_t>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') value |= static_cast<uint64_t>(c - 'A' + 10);
        }

        r.limbs[limb] = value;
        pos -= chunkLen;
    }

    r.Trim();
    return r;
}

std::string BigUint::ToHex() const {

    if (limbs.empty()) return "0";

    std::string out;
    char buf[17];

    for (size_t i = limbs.size(); i-- > 0; ) {

        std::snprintf(buf, sizeof(buf), "%016llX", static_cast<unsigned long long>(limbs[i]));

        if (out.empty()) {
            const char* p = buf;
            while (*p == '0' && *(p + 1) != '\0') ++p;
            out += p;
        }
        else out += buf;
    }

    return out;
}

bool BigUint::IsZero() const {

    return limbs.empty();
}

size_t BigUint::BitLength() const {

    if (limbs.empty()) return 0;

    size_t bits = (limbs.size() - 1) * 64;
    uint64_t top = limbs.back();

    while (top != 0) { ++bits; top >>= 1; }

    return bits;
}

bool BigUint::GetBit(size_t index) const {

    size_t limb = index / 64;
    if (limb >= limbs.size()) return false;

    return (limbs[limb] >> (index % 64)) & 1ULL;
}

std::strong_ordering BigUint::operator<=>(const BigUint& other) const {

    if (limbs.size() != other.limbs.size()) return limbs.size() <=> other.limbs.size();

    for (size_t i = limbs.size(); i-- > 0; ) {
        if (limbs[i] != other.limbs[i]) return limbs[i] <=> other.limbs[i];
    }

    return std::strong_ordering::equal;
}

bool BigUint::operator==(const BigUint& other) const {

    return limbs == other.limbs;
}

BigUint BigUint::operator+(const BigUint& other) const {

    BigUint r;
    size_t n = std::max(limbs.size(), other.limbs.size());
    r.limbs.resize(n, 0);

    unsigned __int128 carry = 0;
    for (size_t i = 0; i < n; ++i) {

        unsigned __int128 sum = carry;
        if (i < limbs.size()) sum += limbs[i];
        if (i < other.limbs.size()) sum += other.limbs[i];

        r.limbs[i] = static_cast<uint64_t>(sum);
        carry = sum >> 64;
    }

    if (carry) r.limbs.push_back(static_cast<uint64_t>(carry));

    r.Trim();
    return r;
}

BigUint BigUint::operator-(const BigUint& other) const {

    BigUint r;
    r.limbs.resize(limbs.size(), 0);

    __int128 borrow = 0;
    for (size_t i = 0; i < limbs.size(); ++i) {

        __int128 diff = static_cast<__int128>(limbs[i]) - borrow - (i < other.limbs.size() ? static_cast<__int128>(other.limbs[i]) : 0);

        if (diff < 0) { diff += (static_cast<__int128>(1) << 64); borrow = 1; }
        else borrow = 0;

        r.limbs[i] = static_cast<uint64_t>(diff);
    }

    r.Trim();
    return r;
}

BigUint BigUint::operator*(const BigUint& other) const {

    BigUint r;
    if (limbs.empty() || other.limbs.empty()) return r;

    r.limbs.assign(limbs.size() + other.limbs.size(), 0);

    for (size_t i = 0; i < limbs.size(); ++i) {

        unsigned __int128 carry = 0;
        for (size_t j = 0; j < other.limbs.size(); ++j) {

            unsigned __int128 cur = static_cast<unsigned __int128>(limbs[i]) * other.limbs[j] + r.limbs[i + j] + carry;

            r.limbs[i + j] = static_cast<uint64_t>(cur);
            carry = cur >> 64;
        }

        size_t k = i + other.limbs.size();
        while (carry) {
            unsigned __int128 cur = static_cast<unsigned __int128>(r.limbs[k]) + carry;
            r.limbs[k] = static_cast<uint64_t>(cur);
            carry = cur >> 64;
            ++k;
        }
    }

    r.Trim();
    return r;
}

BigUint BigUint::ShiftLimbs(size_t k) const {

    if (IsZero() || k == 0) return *this;

    BigUint r;
    r.limbs.assign(k, 0);
    r.limbs.insert(r.limbs.end(), limbs.begin(), limbs.end());

    return r;
}

BigUint BigUint::LowLimbs(size_t k) const {

    BigUint r;
    size_t n = std::min(k, limbs.size());
    r.limbs.assign(limbs.begin(), limbs.begin() + static_cast<long>(n));

    r.Trim();
    return r;
}

std::pair<BigUint, BigUint> DivMod(const BigUint& a, const BigUint& b) {

    BigUint quotient;
    BigUint remainder;

    quotient.limbs.assign(a.limbs.size(), 0);

    for (size_t i = a.BitLength(); i-- > 0; ) {

        remainder = remainder.ShiftLimbs(0);
        BigUint doubled = remainder + remainder;
        remainder = doubled;

        if (a.GetBit(i)) remainder = remainder + BigUint::FromU64(1);

        if (remainder >= b) {
            remainder = remainder - b;
            size_t limb = i / 64;
            if (limb >= quotient.limbs.size()) quotient.limbs.resize(limb + 1, 0);
            quotient.limbs[limb] |= (1ULL << (i % 64));
        }
    }

    quotient.Trim();
    remainder.Trim();

    return { quotient, remainder };
}


Modulus::Modulus(BigUint modulusValue) : n(std::move(modulusValue)) {

    k = n.limbs.size();

    BigUint numerator = BigUint::FromU64(1).ShiftLimbs(2 * k);
    mu = DivMod(numerator, n).first;
}

BigUint Modulus::Reduce(const BigUint& x) const {

    BigUint q1 = x.LowLimbs(x.limbs.size());
    if (q1.limbs.size() > k - 1) q1.limbs.erase(q1.limbs.begin(), q1.limbs.begin() + static_cast<long>(k - 1));
    else q1.limbs.clear();

    BigUint q2 = q1 * mu;

    BigUint q3 = q2;
    if (q3.limbs.size() > k + 1) q3.limbs.erase(q3.limbs.begin(), q3.limbs.begin() + static_cast<long>(k + 1));
    else q3.limbs.clear();

    BigUint r1 = x.LowLimbs(k + 1);
    BigUint r2 = (q3 * n).LowLimbs(k + 1);

    BigUint r;
    if (r1 >= r2) r = r1 - r2;
    else {
        BigUint mod = BigUint::FromU64(1).ShiftLimbs(k + 1);
        r = (mod - r2) + r1;
        r = r.LowLimbs(k + 1);
    }

    while (r >= n) r = r - n;

    return r;
}

BigUint Modulus::MulMod(const BigUint& a, const BigUint& b) const {

    return Reduce(a * b);
}

BigUint Modulus::PowMod(const BigUint& base, const BigUint& exp) const {

    BigUint result = BigUint::FromU64(1);
    BigUint b = (base >= n) ? Reduce(base) : base;

    for (size_t i = exp.BitLength(); i-- > 0; ) {

        result = MulMod(result, result);
        if (exp.GetBit(i)) result = MulMod(result, b);
    }

    return result;
}
