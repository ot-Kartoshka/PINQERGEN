    #include "GeneratorManager.hpp"

#include <algorithm>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <print>
#include <string>


namespace {

    void PrintHelp() {


        std::println("  lab1 -g <датчик> [-n БІТИ] [-a ALPHA]... [-r N] [-t ТЕСТ] [-s SEED] [-o FILE]");
        std::println("  lab1 -f <файл>   [-n БІТИ] [-a ALPHA]... [-r N] [-t ТЕСТ]");
        std::println("  lab1 --experiment [--csv FILE] [-n БІТИ] [-a ALPHA]... [-r N] [-s SEED]");
        std::println("  lab1 --list");
        std::println("");
        std::println("  -g, --generator <код>   датчик з переліку --list (за замовч. builtin)");
        std::println("  -f, --file <шлях>       перевірити послідовність із зовнішнього файлу");
        std::println("  -n, --length <біти>     довжина послідовності в бітах (за замовч. 1000000)");
        std::println("  -a, --alpha <val>       рівень значимості, можна кілька разів");
        std::println("                          (за замовчуванням: 0.01 0.05 0.10)");
        std::println("  -r, --segments <n>      кількість відрізків для тесту однорідності (за замовч. 50)");
        std::println("  -t, --test <тест>       equi | indep | homog | all (за замовч. all)");
        std::println("  -s, --seed <число>      seed генератора (за замовч. від системного годинника)");
        std::println("  -o, --output <шлях>     зберегти згенеровану послідовність у бінарний файл");
        std::println("      --text-file <шлях>  власний текст для генератора \"Бібліотекар\"");
        std::println("      --experiment        прогнати всі 12 датчиків і всі тести, звести в таблицю");
        std::println("      --csv <шлях>         зберегти результати --experiment у CSV");
        std::println("      --list              вивести перелік датчиків");
        std::println("  -h, --help              ця довідка");
    }

    void PrintGeneratorList() {

        std::println("Доступні датчики:");
        for (const auto& info : AllGenerators()) {
            std::println("  {:<10}  {}{}", info.code, info.title, info.isNative8Bit ? "" : "  (бітовий)");
        }
    }

    bool ParseDouble(std::string_view s, double& out) {

        try { out = std::stod(std::string(s)); return true; }
        catch (...) { return false; }
    }

    bool ParseU64(std::string_view s, uint64_t& out) {

        auto res = std::from_chars(s.data(), s.data() + s.size(), out);
        return res.ec == std::errc{};
    }

    bool ParseSize(std::string_view s, size_t& out) {

        uint64_t v;
        if (!ParseU64(s, v)) return false;
        out = static_cast<size_t>(v);
        return true;
    }

}


int main(int argc, char** argv) {

    GeneratorManager::Options opt;
    opt.alphas.clear();

    bool showHelp = (argc == 1);
    bool showList = false;
    bool runExperiment = false;
    std::string csvPath;
    bool generatorGiven = false;
    std::string generatorCode;

    for (int i = 1; i < argc; ++i) {

        std::string_view arg = argv[i];
        auto next = [&]() -> std::string_view { 
            return (i + 1 < argc) ? std::string_view(argv[++i]) : std::string_view{}; 
        };

        if (arg == "-h" || arg == "--help") showHelp = true;
        else if (arg == "--list") showList = true;
        else if (arg == "--experiment") runExperiment = true;
        else if (arg == "--csv") csvPath = next();
        else if (arg == "-g" || arg == "--generator") { generatorCode = next(); generatorGiven = true; }
        else if (arg == "-f" || arg == "--file") { opt.useExternalFile = true; opt.inputFile = next(); }
        else if (arg == "-o" || arg == "--output") opt.outputFile = next();
        else if (arg == "--text-file") opt.textFile = next();
        else if (arg == "-n" || arg == "--length") {
            if (!ParseSize(next(), opt.lengthBits) || opt.lengthBits == 0) {
                std::println("{}", error_to_string(Error::InvalidLength));
                return 1;
            }
        }
        else if (arg == "-a" || arg == "--alpha") {
            double a;
            if (!ParseDouble(next(), a) || a <= 0.0 || a >= 1.0) {
                std::println("{}", error_to_string(Error::InvalidAlpha));
                return 1;
            }
            opt.alphas.push_back(a);
        }
        else if (arg == "-r" || arg == "--segments") {
            if (!ParseSize(next(), opt.segments) || opt.segments < 2) {
                std::println("{}", error_to_string(Error::InvalidSegments));
                return 1;
            }
        }
        else if (arg == "-s" || arg == "--seed") {
            if (!ParseU64(next(), opt.seed)) {
                std::println("{}", error_to_string(Error::ParseError));
                return 1;
            }
            opt.seedProvided = true;
        }
        else if (arg == "-t" || arg == "--test") {
            std::string_view t = next();
            if (t == "equi") opt.test = TestKind::Equiprobability;
            else if (t == "indep") opt.test = TestKind::Independence;
            else if (t == "homog") opt.test = TestKind::Homogeneity;
            else if (t == "all") opt.test = TestKind::All;
            else { std::println("Невідомий тест: {}", t); return 1; }
        }
        else {
            std::println("Нерозпізнаний аргумент: {}", arg);
            return 1;
        }
    }

    if (showHelp) { PrintHelp(); return 0; }
    if (showList) { PrintGeneratorList(); return 0; }

    if (opt.alphas.empty()) opt.alphas = { 0.01, 0.05, 0.10 };
    if (opt.segments == 0) opt.segments = 50;

    if (generatorGiven) {

        bool found = false;
        for (const auto& info : AllGenerators()) {
            if (info.code == generatorCode) { opt.generator = info.id; found = true; break; }
        }

        if (!found) {
            std::println("{}", error_to_string(Error::UnknownGenerator));
            return 1;
        }
    }

    GeneratorManager manager;

    if (runExperiment) {

        auto reports = manager.RunExperiment(opt);

        for (double a : opt.alphas) manager.PrintExperimentSummary(reports, a);

        for (const auto& r : reports) manager.PrintReport(r);

        if (!csvPath.empty()) {

            auto res = manager.ExportCsv(reports, csvPath);
            if (!res) { std::println("{}", error_to_string(res.error())); return 1; }
            std::println("Зведену таблицю збережено у {}", csvPath);
        }

        return 0;
    }

    auto sequence = manager.ObtainSequence(opt);
    if (!sequence) {
        std::println("{}", error_to_string(sequence.error()));
        return 1;
    }

    if (sequence->size() < 50) {
        std::println("{}", error_to_string(Error::NotEnoughData));
        return 1;
    }

    if (!opt.outputFile.empty()) {

        auto res = manager.WriteSequence(opt.outputFile, *sequence);
        if (!res) { std::println("{}", error_to_string(res.error())); return 1; }
        std::println("Послідовність збережено у {}", opt.outputFile);
    }

    GeneratorInfo info = opt.useExternalFile ? GeneratorInfo{ GeneratorId::Builtin, "file", opt.inputFile, true } : *std::find_if(AllGenerators().begin(), AllGenerators().end(), [&](const GeneratorInfo& g) { return g.id == opt.generator; });

    GeneratorReport report = manager.EvaluateGenerator(info, *sequence, opt.alphas, opt.segments, opt.test);
    manager.PrintReport(report);

    return 0;
}
