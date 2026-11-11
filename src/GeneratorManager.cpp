#include "GeneratorManager.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <print>


namespace {

    using Clock = std::chrono::steady_clock;

    std::size_t Utf8Width(const std::string& s) {

        std::size_t n = 0;
        for (unsigned char c : s) if ((c & 0xC0) != 0x80) ++n;
        return n;
    }

    std::string MakeBullets(std::size_t n) {

        std::string r;
        for (std::size_t i = 0; i < n; ++i) {
            if (i) r += ' ';
            r += "\u2022";
        }
        return r;
    }

    std::string MakeBorder(std::size_t width) {

        return MakeBullets((width + 1) / 2);
    }

    std::pair<std::string, std::size_t> MakeSectionTitle(const std::string& title, std::size_t minWidth = 0) {

        std::size_t tv = Utf8Width(title);
        std::size_t minBullets = 3;
        std::size_t l = std::max(minBullets, minWidth > tv ? (minWidth - tv + 4) / 4 : minBullets);
        std::size_t w = 4 * l + tv;

        std::string extra = (w % 2 == 0) ? " " : "";
        if (w % 2 == 0) ++w;

        return { MakeBullets(l) + ' ' + extra + title + ' ' + MakeBullets(l), w };
    }

    std::string Center(const std::string& s, std::size_t w) {

        std::size_t v = Utf8Width(s);
        if (v >= w) return s;
        std::size_t pad = w - v;
        return std::string(pad / 2, ' ') + s + std::string(pad - pad / 2, ' ');
    }

    std::string Ljust(const std::string& s, std::size_t w) {

        std::size_t v = Utf8Width(s);
        return v < w ? s + std::string(w - v, ' ') : s;
    }

    std::string FormatAlpha(double a) {

        char buf[16];
        std::snprintf(buf, sizeof(buf), "%.2f", a);
        return buf;
    }

    std::string FormatNum(double v, int prec = 2) {

        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.*f", prec, v);
        return buf;
    }

    void PrintAlphaRows(const std::vector<AlphaResult>& rows) {

        for (const auto& [alpha, res] : rows) {

            std::println("    alpha={}  l={}  chi2={}  межа={}  ->  {}",
                FormatAlpha(alpha),
                FormatNum(res.degreesL, 1),
                FormatNum(res.statistic, 3),
                FormatNum(res.threshold, 3),
                res.passed ? "H0 приймається" : "H0 ВІДКИДАЄТЬСЯ");
        }
    }

}


std::expected<std::vector<uint8_t>, Error> GeneratorManager::ObtainSequence(const Options& opt, const GeneratorInfo* overrideGenerator) {

    size_t bytesNeeded = opt.lengthBits / 8;
    if (bytesNeeded == 0) return std::unexpected(Error::InvalidLength);

    if (opt.useExternalFile) {

        std::ifstream file(opt.inputFile, std::ios::binary);
        if (!file) return std::unexpected(Error::FileReadError);

        std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        if (data.empty()) return std::unexpected(Error::EmptySequence);

        if (data.size() > bytesNeeded) data.resize(bytesNeeded);
        return data;
    }

    GeneratorId id = overrideGenerator ? overrideGenerator->id : opt.generator;

    uint64_t seed = opt.seedProvided ? opt.seed : static_cast<uint64_t>(Clock::now().time_since_epoch().count());

    auto generator = CreateGenerator(id, seed, opt.textFile);
    if (!generator) return std::unexpected(Error::UnknownGenerator);

    return generator->NextBytes(bytesNeeded);
}

std::expected<void, Error> GeneratorManager::WriteSequence(std::string_view path, std::span<const uint8_t> data) {

    std::ofstream file(std::string(path), std::ios::binary);
    if (!file) return std::unexpected(Error::FileWriteError);

    file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!file) return std::unexpected(Error::FileWriteError);

    return {};
}

GeneratorReport GeneratorManager::EvaluateGenerator(const GeneratorInfo& info, std::span<const uint8_t> data, const std::vector<double>& alphas, size_t segments, TestKind which) {

    GeneratorReport report;
    report.info = info;
    report.bytesUsed = data.size();

    bool doEqui = (which == TestKind::All || which == TestKind::Equiprobability);
    bool doIndep = (which == TestKind::All || which == TestKind::Independence);
    bool doHomog = (which == TestKind::All || which == TestKind::Homogeneity);

    for (double a : alphas) {
        if (doEqui) report.equi.push_back({ a, TestEquiprobability(data, a) });
        if (doIndep) report.indep.push_back({ a, TestIndependence(data, a) });
        if (doHomog) report.homog.push_back({ a, TestHomogeneity(data, segments, a) });
    }

    return report;
}

void GeneratorManager::PrintReport(const GeneratorReport& report) const {

    std::string header = std::string(report.info.title) + " -- n = " + std::to_string(report.bytesUsed) + " байт";
    auto [titleLine, w] = MakeSectionTitle(header, 60);

    std::println("{}", titleLine);

    if (!report.equi.empty()) { std::println("  Критерій рівноімовірності знаків:"); PrintAlphaRows(report.equi); }
    if (!report.indep.empty()) { std::println("  Критерій незалежності сусідніх знаків:"); PrintAlphaRows(report.indep); }
    if (!report.homog.empty()) { std::println("  Критерій однорідності послідовності:"); PrintAlphaRows(report.homog); }

    std::println("{}", MakeBorder(w));
}

std::vector<GeneratorReport> GeneratorManager::RunExperiment(const Options& opt) {

    std::vector<GeneratorReport> reports;

    auto [titleLine, w] = MakeSectionTitle("Повний прогін: усі датчики x усі тести", 60);
    std::println("{}", titleLine);

    for (const GeneratorInfo& info : AllGenerators()) {

        std::print("  -> {} ... ", info.title);
        std::fflush(stdout);

        auto start = Clock::now();
        auto seq = ObtainSequence(opt, &info);
        double genMs = std::chrono::duration<double, std::milli>(Clock::now() - start).count();

        if (!seq) {
            std::println("ПОМИЛКА: {}", error_to_string(seq.error()));
            continue;
        }

        GeneratorReport report = EvaluateGenerator(info, *seq, opt.alphas, opt.segments, opt.test);
        report.generationMs = genMs;

        std::println("готово ({} мс, {} байт)", FormatNum(genMs, 0), report.bytesUsed);

        reports.push_back(std::move(report));
    }

    std::println("{}", MakeBorder(w));

    return reports;
}

void GeneratorManager::PrintExperimentSummary(const std::vector<GeneratorReport>& reports, double alpha) const {

    auto findAt = [](const std::vector<AlphaResult>& v, double a) -> const TestResult* {
        for (const auto& e : v) if (e.alpha == a) return &e.result;
        return nullptr;
        };

    std::string h_gen = "Датчик";
    std::string h_equi = "Рівноімов.";
    std::string h_indep = "Незалежн.";
    std::string h_homog = "Однорідн.";

    size_t wGen = h_gen.size();
    for (const auto& r : reports) wGen = std::max(wGen, Utf8Width(std::string(r.info.title)));

    size_t wCol = 12;

    auto [titleLine, w] = MakeSectionTitle("Зведена таблиця, alpha = " + FormatAlpha(alpha), wGen + 3 * (wCol + 2) + 8);

    std::println("{}", titleLine);
    std::println("{}  {}  {}  {}", Ljust(h_gen, wGen), Center(h_equi, wCol), Center(h_indep, wCol), Center(h_homog, wCol));
    std::println("{}", std::string(wGen + 3 * (wCol + 2), '-'));

    auto verdict = [&](const TestResult* r) {
        if (!r) return std::string("  --  ");
        return std::string(r->passed ? "  OK  " : " ВІДХ.");
        };

    for (const auto& r : reports) {

        const TestResult* e = findAt(r.equi, alpha);
        const TestResult* i = findAt(r.indep, alpha);
        const TestResult* h = findAt(r.homog, alpha);

        std::println("{}  {}  {}  {}", Ljust(std::string(r.info.title), wGen), Center(verdict(e), wCol), Center(verdict(i), wCol), Center(verdict(h), wCol));
    }

    std::println("{}", MakeBorder(w));
}

std::expected<void, Error> GeneratorManager::ExportCsv(const std::vector<GeneratorReport>& reports, std::string_view path) const {

    std::ofstream file{ std::string(path) };
    if (!file) return std::unexpected(Error::FileWriteError);

    file << "generator,test,alpha,n,l,chi2,threshold,verdict\n";

    auto writeRows = [&](const GeneratorReport& r, std::string_view testName, const std::vector<AlphaResult>& rows) {
        for (const auto& [alpha, res] : rows) {
            file << r.info.code << ',' << testName << ',' << FormatAlpha(alpha) << ',' << r.bytesUsed << ',' << FormatNum(res.degreesL, 1) << ',' << FormatNum(res.statistic, 4) << ',' << FormatNum(res.threshold, 4) << ',' << (res.passed ? "OK" : "REJECTED") << '\n';
        }
        };

    for (const auto& r : reports) {
        writeRows(r, "equiprobability", r.equi);
        writeRows(r, "independence", r.indep);
        writeRows(r, "homogeneity", r.homog);
    }

    if (!file) return std::unexpected(Error::FileWriteError);

    return {};
}
