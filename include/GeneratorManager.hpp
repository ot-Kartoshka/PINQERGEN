#pragma once

#include <expected>
#include <span>
#include <string>
#include <vector>

#include "Errors.hpp"
#include "Generators.hpp"
#include "Statistics.hpp"


enum class TestKind { All, Equiprobability, Independence, Homogeneity };

struct AlphaResult {
    double alpha;
    TestResult result;
};

struct GeneratorReport {
    GeneratorInfo info;
    size_t bytesUsed = 0;
    double generationMs = 0.0;
    std::vector<AlphaResult> equi;
    std::vector<AlphaResult> indep;
    std::vector<AlphaResult> homog;
};


class GeneratorManager {

public:

    struct Options {
        GeneratorId generator = GeneratorId::Builtin;
        bool useExternalFile = false;
        std::string inputFile;
        std::string outputFile;
        std::string textFile;
        uint64_t seed = 0;
        bool seedProvided = false;
        size_t lengthBits = 1e6;
        std::vector<double> alphas{ 0.01, 0.05, 0.10 };
        size_t segments = 50;
        TestKind test = TestKind::All;
    };

    std::expected<std::vector<uint8_t>, Error> ObtainSequence(const Options& opt, const GeneratorInfo* overrideGenerator = nullptr);

    std::expected<void, Error> WriteSequence(std::string_view path, std::span<const uint8_t> data);

    GeneratorReport EvaluateGenerator(const GeneratorInfo& info, std::span<const uint8_t> data, const std::vector<double>& alphas, size_t segments, TestKind which);

    void PrintReport(const GeneratorReport& report) const;

    std::vector<GeneratorReport> RunExperiment(const Options& opt);

    void PrintExperimentSummary(const std::vector<GeneratorReport>& reports, double alpha) const;

    std::expected<void, Error> ExportCsv(const std::vector<GeneratorReport>& reports, std::string_view path) const;
};
