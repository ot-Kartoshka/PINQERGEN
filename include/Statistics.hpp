#pragma once

#include <cstdint>
#include <span>
#include <cstddef>


double NormalQuantile(double p);


double ChiSquareThreshold(double l, double alpha);


struct TestResult {

    double statistic = 0.0;
    double threshold = 0.0;
    double degreesL = 0.0;
    size_t samplesUsed = 0;
    bool passed = false;
};


TestResult TestEquiprobability(std::span<const uint8_t> data, double alpha);

TestResult TestIndependence(std::span<const uint8_t> data, double alpha);

TestResult TestHomogeneity(std::span<const uint8_t> data, size_t segments, double alpha);
