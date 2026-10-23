// dual_bench: runs one dualization algorithm on one matrix (a .hg file or a random matrix) with a
// time limit and prints the result as one JSON line. Driven by benchmarks/run_benchmarks.py (#48).
//
//   dual_bench --algorithm RUNCM --random M N P SEED [--time-limit SEC]
//   dual_bench --algorithm FK --file matrix.hg [--time-limit SEC]
//   dual_bench --list | --info | --help

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <functional>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <dual/aot1.h>
#include <dual/fk.h>
#include <dual/punc.h>
#include <dual/runc.h>
#include <dual/runc_fixed.h>

using namespace dual;

namespace {

typedef std::vector<std::vector<int>> Rows;
typedef std::chrono::steady_clock Clock;

// Callbacks check the clock only every this many calls.
const int checkEvery = 1024;

struct Matrix {
    Rows rows;
    int n = 0;
};

struct Options {
    std::string algorithm;
    std::string file;
    bool random = false;
    int m = 0;
    int n = 0;
    double p = 0;
    std::uint64_t seed = 0;
    double timeLimit = 10;
};

struct Result {
    long long covers = 0;
    double seconds = 0;
    bool timedOut = false;
};

// Random m x n matrix: every cell is 1 with probability p, independently; a row that came out
// empty gets one 1 in a uniformly chosen column, so the matrix has no zero rows. Uses only the raw
// mt19937_64 output (no std distributions), so a seed gives the same matrix with every compiler.
Matrix RandomMatrix(int m, int n, double p, std::uint64_t seed) {
    std::mt19937_64 random(seed);
    // P(x < threshold) = p for x uniform in [0, 2^64), up to rounding of p * 2^64.
    double scaled = p * 18446744073709551616.0;
    std::uint64_t threshold = scaled >= 18446744073709551615.0 ? UINT64_MAX : (std::uint64_t) scaled;
    Matrix matrix;
    matrix.n = n;
    matrix.rows.resize(m);
    for (auto& row : matrix.rows) {
        for (int j = 0; j < n; ++j)
            if (random() < threshold)
                row.push_back(j);
        if (row.empty())
            row.push_back((int) (random() % (std::uint64_t) n));
    }
    return matrix;
}

Matrix FileMatrix(std::string const& file) {
    Runc::RuncGlobalState reader;
    reader.loadMatrix(file);
    auto const& bits = reader.get_matrix();
    Matrix matrix;
    matrix.n = bits.width();
    matrix.rows.resize(bits.height());
    for (int i = 0; i < bits.height(); ++i) {
        auto const& row = bits.get_row(i);
        for (int j = row.find_first(); j >= 0; j = row.find_next(j))
            matrix.rows[i].push_back(j);
    }
    return matrix;
}

bit_matrix ToBitMatrix(Matrix const& matrix) {
    bit_matrix bits((int) matrix.rows.size(), matrix.n);
    for (int i = 0; i < (int) matrix.rows.size(); ++i)
        for (int j : matrix.rows[i])
            bits.get_row(i).set(j);
    return bits;
}

// Counts covers and stops the enumeration once the deadline has passed.
class Watch : public IDualizationCallback {
public:
    explicit Watch(Clock::time_point deadline) : deadline_(deadline) {}

    void Call(DualizationNode& node) override {
        if (node.IsCover())
            ++covers;
        if (++calls_ % checkEvery == 0 && Clock::now() >= deadline_) {
            node.stoped = true;
            timedOut = true;
        }
    }

    long long covers = 0;
    bool timedOut = false;

private:
    Clock::time_point deadline_;
    long long calls_ = 0;
};

Result RunBacktrack(DualizationBacktrackAlgorithmBase& algorithm, Matrix const& matrix, double timeLimit) {
    algorithm.setMatrix(ToBitMatrix(matrix));
    auto start = Clock::now();
    auto deadline = start + std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(timeLimit));
    Watch covers(deadline);
    Watch inner(deadline);
    algorithm.CoverCallback = &covers;
    algorithm.InnerCallback = &inner;
    algorithm.dualize();
    Result result;
    result.seconds = std::chrono::duration<double>(Clock::now() - start).count();
    result.covers = covers.covers;
    result.timedOut = covers.timedOut || inner.timedOut;
    algorithm.CoverCallback = nullptr;
    algorithm.InnerCallback = nullptr;
    return result;
}

template<typename Row>
Result RunFixed(Matrix const& matrix, double timeLimit) {
    std::vector<Row> rows;
    rows.reserve(matrix.rows.size());
    for (auto const& r : matrix.rows) {
        Row row = 0;
        for (int j : r)
            row |= Row(1) << j;
        rows.push_back(row);
    }
    RuncFixed<Row> runc;
    runc.callbackDelay = checkEvery;
    auto start = Clock::now();
    auto deadline = start + std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(timeLimit));
    bool timedOut = false;
    // Called every callbackDelay covers; returning false stops the enumeration.
    runc.callback = [deadline, &timedOut](RuncFixed<Row>*) {
        timedOut = Clock::now() >= deadline;
        return !timedOut;
    };
    runc.Dualize(std::move(rows));
    Result result;
    result.seconds = std::chrono::duration<double>(Clock::now() - start).count();
    result.covers = (long long) runc.idealIndep.size();
    result.timedOut = timedOut;
    return result;
}

struct Algorithm {
    const char* name;
    const char* description;
    std::function<Result(Matrix const&, double)> run;
};

template<typename Make>
std::function<Result(Matrix const&, double)> Backtrack(Make make) {
    return [make](Matrix const& matrix, double timeLimit) {
        auto algorithm = make(matrix);
        return RunBacktrack(*algorithm, matrix, timeLimit);
    };
}

std::unique_ptr<DualizationBacktrackAlgorithmBase> MakeRunc(Runc::AlgorithmType type) {
    return std::make_unique<Runc::RuncGlobalState>(type);
}

std::unique_ptr<DualizationBacktrackAlgorithmBase> MakeAot1(AOT1::AlgorithmType type) {
    return std::make_unique<AOT1::GlobalState>(type);
}

// PUNC with rows covered in increasing order of their number of ones and, if byColumns, columns
// tried in decreasing order of theirs.
std::unique_ptr<DualizationBacktrackAlgorithmBase> MakePunc(Matrix const& matrix, bool byRows, bool byColumns) {
    auto punc = std::make_unique<PUNC::GlobalState>();
    punc->setMatrix(ToBitMatrix(matrix));
    if (byRows) {
        Weights rowWeights;
        for (auto const& row : matrix.rows)
            rowWeights.push_back((Weight) row.size());
        punc->SetRowsOrder(rowWeights);
    }
    if (byColumns) {
        Weights colWeights(matrix.n, 0);
        for (auto const& row : matrix.rows)
            for (int j : row)
                colWeights[j] -= 1;
        punc->SetColsOrder(colWeights);
    }
    return punc;
}

std::vector<Algorithm> Algorithms() {
    return {
            {"RUNCM",
             "RUNC-M: branch on the uncovered row with the fewest remaining columns",
             Backtrack([](Matrix const&) { return MakeRunc(Runc::RUNCM); })},
            {"RUNC", "RUNC: branch on the first uncovered row", Backtrack([](Matrix const&) {
                 return MakeRunc(Runc::RUNC);
             })},
            {"OPT", "OPT: branch on all remaining columns after removing dependent rows", Backtrack([](Matrix const&) {
                 return MakeRunc(Runc::OPT);
             })},
            {"RuncFixed",
             "RuncFixed: RUNC-M on machine-word rows (n <= 64)",
             [](Matrix const& matrix, double timeLimit) {
                 if (matrix.n > 64)
                     throw std::invalid_argument("RuncFixed supports at most 64 columns");
                 return matrix.n <= 32 ? RunFixed<bits32>(matrix, timeLimit) : RunFixed<bits64>(matrix, timeLimit);
             }},
            {"AO1", "AOT1 AO1: all remaining columns", Backtrack([](Matrix const&) { return MakeAot1(AOT1::AO1); })},
            {"AO2", "AOT1 AO2: AO1 after removing dependent rows", Backtrack([](Matrix const&) {
                 return MakeAot1(AOT1::AO2);
             })},
            {"AO1K", "AOT1 AO1K: columns of the first competing row", Backtrack([](Matrix const&) {
                 return MakeAot1(AOT1::AO1K);
             })},
            {"AO2K", "AOT1 AO2K: AO1K after removing dependent rows", Backtrack([](Matrix const&) {
                 return MakeAot1(AOT1::AO2K);
             })},
            {"AO1M", "AOT1 AO1M: columns of the row with the fewest remaining columns", Backtrack([](Matrix const&) {
                 return MakeAot1(AOT1::AO1M);
             })},
            {"AO2M", "AOT1 AO2M: AO1M after removing dependent rows", Backtrack([](Matrix const&) {
                 return MakeAot1(AOT1::AO2M);
             })},
            {"PUNC", "PUNC: rows in input order, columns in random order", Backtrack([](Matrix const& matrix) {
                 return MakePunc(matrix, false, false);
             })},
            {"PUNC-R", "PUNC: rows by increasing number of ones", Backtrack([](Matrix const& matrix) {
                 return MakePunc(matrix, true, false);
             })},
            {"PUNC-RC",
             "PUNC: rows by increasing, columns by decreasing number of ones",
             Backtrack([](Matrix const& matrix) { return MakePunc(matrix, true, true); })},
            {"FK", "Fredman-Khachiyan incremental dualization (FK-B)", Backtrack([](Matrix const&) {
                 return std::make_unique<FK::GlobalState>();
             })},
    };
}

std::string Escape(std::string const& s) {
    std::string out;
    for (char c : s) {
        if (c == '"' || c == '\\')
            out += '\\';
        if ((unsigned char) c < 0x20)
            out += ' ';
        else
            out += c;
    }
    return out;
}

void PrintInfo() {
#if defined(__clang__)
    std::string compiler = std::string("clang ") + __clang_version__;
#elif defined(__GNUC__)
    std::string compiler = std::string("gcc ") + __VERSION__;
#elif defined(_MSC_VER)
    std::string compiler = "msvc " + std::to_string(_MSC_FULL_VER);
#else
    std::string compiler = "unknown";
#endif
#ifdef NDEBUG
    const char* assertions = "off";
#else
    const char* assertions = "on";
#endif
    std::printf("{\"compiler\": \"%s\", \"assertions\": \"%s\", \"pointer_bits\": %d}\n",
                Escape(compiler).c_str(),
                assertions,
                (int) (8 * sizeof(void*)));
}

void PrintUsage() {
    std::printf("Usage:\n"
                "  dual_bench --algorithm NAME (--random M N P SEED | --file FILE.hg) [--time-limit SEC]\n"
                "  dual_bench --list     list the algorithms\n"
                "  dual_bench --info     print compiler and build information as JSON\n"
                "Prints one JSON line: algorithm, rows, cols, density, seed, file, covers, time_s,\n"
                "timed_out, status (ok | timeout | error) and error. --random makes an M x N matrix\n"
                "whose cells are 1 with probability P (a zero row gets one random 1).\n");
}

double ToDouble(const char* s) {
    char* end = nullptr;
    double v = std::strtod(s, &end);
    if (end == s || *end != '\0')
        throw std::invalid_argument(std::string("not a number: ") + s);
    return v;
}

long long ToInt(const char* s) {
    char* end = nullptr;
    long long v = std::strtoll(s, &end, 10);
    if (end == s || *end != '\0')
        throw std::invalid_argument(std::string("not an integer: ") + s);
    return v;
}

Options ParseArgs(int argc, char** argv) {
    Options o;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto need = [&](int k) {
            if (i + k >= argc)
                throw std::invalid_argument(a + " needs " + std::to_string(k) + " argument(s)");
        };
        if (a == "--algorithm") {
            need(1);
            o.algorithm = argv[++i];
        } else if (a == "--file") {
            need(1);
            o.file = argv[++i];
        } else if (a == "--random") {
            need(4);
            o.random = true;
            o.m = (int) ToInt(argv[++i]);
            o.n = (int) ToInt(argv[++i]);
            o.p = ToDouble(argv[++i]);
            o.seed = (std::uint64_t) ToInt(argv[++i]);
        } else if (a == "--time-limit") {
            need(1);
            o.timeLimit = ToDouble(argv[++i]);
        } else {
            throw std::invalid_argument("unknown argument: " + a);
        }
    }
    if (o.algorithm.empty())
        throw std::invalid_argument("--algorithm is required");
    if (o.random == !o.file.empty())
        throw std::invalid_argument("give exactly one of --random and --file");
    if (o.random && (o.m < 1 || o.n < 1 || o.p < 0 || o.p > 1))
        throw std::invalid_argument("--random needs M >= 1, N >= 1 and 0 <= P <= 1");
    if (o.timeLimit <= 0)
        throw std::invalid_argument("--time-limit must be positive");
    return o;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--list") {
        for (auto const& a : Algorithms())
            std::printf("%-10s %s\n", a.name, a.description);
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "--info") {
        PrintInfo();
        return 0;
    }
    if (argc < 2 || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
        PrintUsage();
        return argc < 2 ? 2 : 0;
    }

    Options o;
    try {
        o = ParseArgs(argc, argv);
    }
    catch (std::exception const& e) {
        std::fprintf(stderr, "dual_bench: %s\n", e.what());
        PrintUsage();
        return 2;
    }

    Result result;
    std::string status = "ok";
    std::string error;
    Matrix matrix;
    try {
        matrix = o.random ? RandomMatrix(o.m, o.n, o.p, o.seed) : FileMatrix(o.file);
        Algorithm const* algorithm = nullptr;
        auto algorithms = Algorithms();
        for (auto const& a : algorithms)
            if (o.algorithm == a.name)
                algorithm = &a;
        if (!algorithm)
            throw std::invalid_argument("unknown algorithm: " + o.algorithm + " (see --list)");
        result = algorithm->run(matrix, o.timeLimit);
        if (result.timedOut)
            status = "timeout";
    }
    catch (std::exception const& e) {
        status = "error";
        error = e.what();
    }

    std::printf("{\"algorithm\": \"%s\", \"rows\": %d, \"cols\": %d, \"density\": %.6g, \"seed\": %llu, "
                "\"file\": \"%s\", \"covers\": %lld, \"time_s\": %.9f, \"timed_out\": %s, \"status\": \"%s\", "
                "\"error\": \"%s\"}\n",
                Escape(o.algorithm).c_str(),
                (int) matrix.rows.size(),
                matrix.n,
                o.random ? o.p : 0.0,
                (unsigned long long) o.seed,
                Escape(o.file).c_str(),
                result.covers,
                result.seconds,
                result.timedOut ? "true" : "false",
                status.c_str(),
                Escape(error).c_str());
    return status == "error" ? 1 : 0;
}
