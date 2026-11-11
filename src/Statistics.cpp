#include "Statistics.hpp"

#include <array>
#include <cmath>
#include <vector>


double NormalQuantile(double p) {

    static constexpr std::array<double, 6> a{ -3.969683028665376e+01, 2.209460984245205e+02, -2.759285104469687e+02, 1.383577518672690e+02, -3.066479806614716e+01, 2.506628277459239e+00 };

    static constexpr std::array<double, 5> b{ -5.447609879822406e+01, 1.615858368580409e+02, -1.556989798598866e+02, 6.680131188771972e+01, -1.328068155288572e+01 };

    static constexpr std::array<double, 6> c{ -7.784894002430293e-03, -3.223964580411365e-01, -2.400758277161838e+00, -2.549732539343734e+00, 4.374664141464968e+00, 2.938163982698783e+00 };

    static constexpr std::array<double, 4> d{ 7.784695709041462e-03, 3.224671290700398e-01, 2.445134137142996e+00, 3.754408661907416e+00 };

    constexpr double pLow = 0.02425;
    constexpr double pHigh = 1.0 - pLow;

    if (p < pLow) {

        double q = std::sqrt(-2.0 * std::log(p));
        return (((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5]) /
            ((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + 1.0);
    }

    if (p <= pHigh) {

        double q = p - 0.5;
        double r = q * q;
        return (((((a[0] * r + a[1]) * r + a[2]) * r + a[3]) * r + a[4]) * r + a[5]) * q /
            (((((b[0] * r + b[1]) * r + b[2]) * r + b[3]) * r + b[4]) * r + 1.0);
    }

    double q = std::sqrt(-2.0 * std::log(1.0 - p));
    return -(((((c[0] * q + c[1]) * q + c[2]) * q + c[3]) * q + c[4]) * q + c[5]) /
        ((((d[0] * q + d[1]) * q + d[2]) * q + d[3]) * q + 1.0);
}

double ChiSquareThreshold(double l, double alpha) {

    double z = NormalQuantile(1.0 - alpha);
    return std::sqrt(2.0 * l) * z + l;
}


namespace {

    double ContingencyChiSquare(const std::vector<std::vector<uint64_t>>& v,
        const std::vector<uint64_t>& rowSum,
        const std::vector<uint64_t>& colSum,
        uint64_t n) {

        double sum = 0.0;

        for (size_t i = 0; i < v.size(); ++i) {

            if (rowSum[i] == 0) continue;

            for (size_t j = 0; j < v[i].size(); ++j) {

                if (colSum[j] == 0) continue;

                double vij = static_cast<double>(v[i][j]);
                sum += (vij * vij) / (static_cast<double>(rowSum[i]) * static_cast<double>(colSum[j]));
            }
        }

        return static_cast<double>(n) * (sum - 1.0);
    }

}


TestResult TestEquiprobability(std::span<const uint8_t> data, double alpha) {

    std::array<uint64_t, 256> counts{};
    for (uint8_t byte : data) ++counts[byte];

    double expected = static_cast<double>(data.size()) / 256.0;

    double chi2 = 0.0;
    for (uint64_t v : counts) {
        double diff = static_cast<double>(v) - expected;
        chi2 += (diff * diff) / expected;
    }

    TestResult r;
    r.statistic = chi2;
    r.degreesL = 255.0;
    r.threshold = ChiSquareThreshold(r.degreesL, alpha);
    r.samplesUsed = data.size();
    r.passed = (chi2 <= r.threshold);

    return r;
}

TestResult TestIndependence(std::span<const uint8_t> data, double alpha) {

    size_t pairs = data.size() / 2;

    std::vector<std::vector<uint64_t>> v(256, std::vector<uint64_t>(256, 0));
    std::vector<uint64_t> rowSum(256, 0);
    std::vector<uint64_t> colSum(256, 0);

    for (size_t i = 0; i < pairs; ++i) {

        uint8_t first = data[2 * i];
        uint8_t second = data[2 * i + 1];

        ++v[first][second];
        ++rowSum[first];
        ++colSum[second];
    }

    TestResult r;
    r.statistic = ContingencyChiSquare(v, rowSum, colSum, pairs);
    r.degreesL = 255.0 * 255.0;
    r.threshold = ChiSquareThreshold(r.degreesL, alpha);
    r.samplesUsed = pairs * 2;
    r.passed = (r.statistic <= r.threshold);

    return r;
}

TestResult TestHomogeneity(std::span<const uint8_t> data, size_t segments, double alpha) {

    size_t segmentLen = data.size() / segments;
    size_t used = segmentLen * segments;

    std::vector<std::vector<uint64_t>> v(256, std::vector<uint64_t>(segments, 0));
    std::vector<uint64_t> rowSum(256, 0);
    std::vector<uint64_t> colSum(segments, segmentLen);

    for (size_t j = 0; j < segments; ++j) {
        for (size_t k = 0; k < segmentLen; ++k) {

            uint8_t byte = data[j * segmentLen + k];
            ++v[byte][j];
            ++rowSum[byte];
        }
    }

    TestResult r;
    r.statistic = ContingencyChiSquare(v, rowSum, colSum, used);
    r.degreesL = 255.0 * static_cast<double>(segments - 1);
    r.threshold = ChiSquareThreshold(r.degreesL, alpha);
    r.samplesUsed = used;
    r.passed = (r.statistic <= r.threshold);

    return r;
}
