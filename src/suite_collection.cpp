#include "suite_collection.h"
#include "support.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <regex>
#include <stdexcept>
#include <string>
#include <utility>

namespace FuncCraft {
namespace {

namespace fs = std::filesystem;

struct CollectionRecord {
    SuiteCollectionId id;
    fs::path path;
};

fs::path default_suite_dir() {
#ifdef FUNCCRAFT_DEFAULT_SUITE_DIR
    return fs::path(FUNCCRAFT_DEFAULT_SUITE_DIR);
#else
    return {};
#endif
}

std::vector<fs::path> candidate_suite_dirs() {
    std::vector<fs::path> dirs;

    if (const char* env = std::getenv("FUNCCRAFT_SUITE_DIR")) {
        if (*env != '\0') {
            dirs.emplace_back(env);
        }
    }

    dirs.push_back(fs::current_path() / "suites");
    dirs.push_back(fs::current_path() / ".." / "suites");

    const fs::path compiled = default_suite_dir();
    if (!compiled.empty()) {
        dirs.push_back(compiled);
    }

    std::vector<fs::path> unique_dirs;
    for (const fs::path& dir : dirs) {
        const fs::path normalized = dir.lexically_normal();
        if (std::find(unique_dirs.begin(), unique_dirs.end(), normalized) == unique_dirs.end()) {
            unique_dirs.push_back(normalized);
        }
    }
    return unique_dirs;
}

bool parse_collection_filename(const fs::path& path, int& year, int& version) {
    static const std::regex pattern(R"(^([0-9]{4})_v([0-9]+)\.ya?ml$)", std::regex::icase);
    std::smatch match;
    const std::string name = path.filename().string();
    if (!std::regex_match(name, match, pattern)) {
        return false;
    }
    year = std::stoi(match[1].str());
    version = std::stoi(match[2].str());
    return true;
}

std::vector<CollectionRecord> discover_collections() {
    std::vector<CollectionRecord> records;
    for (const fs::path& dir : candidate_suite_dirs()) {
        if (!fs::exists(dir) || !fs::is_directory(dir)) {
            continue;
        }

        for (const fs::directory_entry& entry : fs::directory_iterator(dir)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            int year = 0;
            int version = 0;
            if (!parse_collection_filename(entry.path(), year, version)) {
                continue;
            }

            const auto duplicate = std::find_if(
                records.begin(),
                records.end(),
                [year, version](const CollectionRecord& record) {
                    return record.id.year == year && record.id.version == version;
                });
            if (duplicate != records.end()) {
                continue;
            }

            SuiteSpec spec = load_suite_spec(entry.path().string());
            records.push_back({{year, version, spec.suite_label}, entry.path()});
        }
    }

    std::sort(
        records.begin(),
        records.end(),
        [](const CollectionRecord& lhs, const CollectionRecord& rhs) {
            if (lhs.id.year != rhs.id.year) {
                return lhs.id.year < rhs.id.year;
            }
            return lhs.id.version < rhs.id.version;
        });
    return records;
}

CollectionRecord find_collection(int year, int version) {
    const std::vector<CollectionRecord> records = discover_collections();
    const auto it = std::find_if(
        records.begin(),
        records.end(),
        [year, version](const CollectionRecord& record) {
            return record.id.year == year && record.id.version == version;
        });
    detail::require(
        it != records.end(),
        "unsupported FuncCraft suite collection: " + std::to_string(year) + "_v" + std::to_string(version));
    return *it;
}

} // namespace

SuiteCollection::SuiteCollection(int year, int version)
    : year_(year),
      version_(version),
      name_(find_collection(year, version).id.name) {}

int SuiteCollection::year() const {
    return year_;
}

int SuiteCollection::version() const {
    return version_;
}

const std::string& SuiteCollection::name() const {
    return name_;
}

int SuiteCollection::number_of_function() const {
    return number_of_functions();
}

int SuiteCollection::number_of_functions() const {
    return suite_collection_number_of_functions(year_, version_);
}

SuiteSpec SuiteCollection::spec() const {
    return suite_collection_spec(year_, version_);
}

BenchmarkSuite SuiteCollection::benchmark_suite(int dimension) const {
    return BenchmarkSuite(spec(), dimension);
}

std::vector<SuiteCollectionId> list_suite_collections() {
    const std::vector<CollectionRecord> records = discover_collections();
    std::vector<SuiteCollectionId> ids;
    ids.reserve(records.size());
    for (const CollectionRecord& record : records) {
        ids.push_back(record.id);
    }
    return ids;
}

SuiteCollection suite_collection(int year, int version) {
    return SuiteCollection(year, version);
}

SuiteSpec suite_collection_spec(int year, int version) {
    return load_suite_spec(find_collection(year, version).path.string());
}

int suite_collection_number_of_functions(int year, int version) {
    return suite_collection_spec(year, version).requested_number_of_functions;
}

int suite_collection_number_of_function(int year, int version) {
    return suite_collection_number_of_functions(year, version);
}

BenchmarkSuite suite_collection_benchmark_suite(
    int year,
    int version,
    int dimension) {
    return SuiteCollection(year, version).benchmark_suite(dimension);
}

} // namespace FuncCraft
