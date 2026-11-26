#include "workload.h"
#include "event.h"
#include "process.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <random>
#include <set>
#include <sstream>
#include "energy_params.h"

static unsigned report_seed_offset = 0;
static bool report_dump = false;
static std::vector<std::string> class_types;

[[noreturn]] static void inputError(const std::string& path, int line, const std::string& msg) {
    fprintf(stderr, "input error: %s:%d: %s\n", path.c_str(), line, msg.c_str());
    exit(2);
}

static std::string trim(const std::string& s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

static std::string normalizeKey(const std::string& s) {
    std::string out;
    bool space = false;
    for (char c : trim(s)) {
        if (std::isspace(static_cast<unsigned char>(c))) { space = true; continue; }
        if (space && !out.empty()) out += ' ';
        space = false;
        out += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return out;
}

static std::uint64_t parseUnsigned(const std::string& path, int line, const std::string& key,
                                   const std::string& value) {
    std::string v = trim(value);
    if (v.empty() || !std::all_of(v.begin(), v.end(), ::isdigit))
        inputError(path, line, "'" + key + "' needs a non-negative integer, got '" + v + "'");
    return std::stoull(v);
}

static void parseRange(const std::string& path, int line, const std::string& key,
                       const std::string& value, int& lo, int& hi) {
    std::string v = trim(value);
    if (!v.empty() && v.front() == '[') {
        if (v.back() != ']')
            inputError(path, line, "'" + key + "' range is missing ']'");
        std::string inner = v.substr(1, v.size() - 2);
        size_t comma = inner.find(',');
        if (comma == std::string::npos)
            inputError(path, line, "'" + key + "' range needs the form [min, max]");
        lo = static_cast<int>(parseUnsigned(path, line, key, inner.substr(0, comma)));
        hi = static_cast<int>(parseUnsigned(path, line, key, inner.substr(comma + 1)));
    } else {
        lo = hi = static_cast<int>(parseUnsigned(path, line, key, v));
    }
    if (lo > hi)
        inputError(path, line, "'" + key + "' has min greater than max");
}

static ProcessClass buildClass(const std::string& path, int header_line,
                               const std::map<std::string, std::pair<int, std::string>>& fields) {
    static const std::set<std::string> known = {
        "start time", "end time", "inter arrival", "batch size", "runtime",
        "io count", "io gap", "io duration", "process type", "seed"};
    static const std::set<std::string> required = {
        "start time", "end time", "inter arrival", "runtime", "seed"};

    for (const auto& f : fields)
        if (!known.count(f.first))
            inputError(path, f.second.first, "unknown field '" + f.first + "'");
    for (const auto& r : required)
        if (!fields.count(r))
            inputError(path, header_line, "process class is missing '" + r + "'");

    ProcessClass pc;
    pc.line = header_line;
    auto get = [&](const std::string& k) -> const std::pair<int, std::string>& { return fields.at(k); };

    pc.start_time    = parseUnsigned(path, get("start time").first, "start time", get("start time").second);
    pc.end_time      = parseUnsigned(path, get("end time").first, "end time", get("end time").second);
    pc.inter_arrival = parseUnsigned(path, get("inter arrival").first, "inter arrival", get("inter arrival").second);
    pc.seed = static_cast<unsigned>(parseUnsigned(path, get("seed").first, "seed", get("seed").second));
    parseRange(path, get("runtime").first, "runtime", get("runtime").second, pc.runtime_min, pc.runtime_max);

    if (fields.count("batch size"))
        pc.batch_size = static_cast<int>(parseUnsigned(path, get("batch size").first, "batch size",
                                                       get("batch size").second));
    if (fields.count("io count"))
        parseRange(path, get("io count").first, "io count", get("io count").second,
                   pc.io_count_min, pc.io_count_max);
    if (pc.io_count_max > 0) {
        if (!fields.count("io gap") || !fields.count("io duration"))
            inputError(path, header_line, "classes with I/O need 'IO gap' and 'IO duration'");
        parseRange(path, get("io gap").first, "io gap", get("io gap").second, pc.io_gap_min, pc.io_gap_max);
        parseRange(path, get("io duration").first, "io duration", get("io duration").second,
                   pc.io_dur_min, pc.io_dur_max);
    }
    if (fields.count("process type")) {
        pc.type = trim(get("process type").second);
        if (pc.type.empty() || pc.type.find_first_of(" \t") != std::string::npos)
            inputError(path, get("process type").first, "'process type' must be a single word");
    }

    if (pc.end_time < pc.start_time)
        inputError(path, header_line, "'end time' is before 'start time'");
    if (pc.inter_arrival == 0)
        inputError(path, header_line, "'inter arrival' must be at least 1");
    if (pc.batch_size < 1)
        inputError(path, header_line, "'batch size' must be at least 1");
    if (pc.runtime_min < 1)
        inputError(path, header_line, "'runtime' must be at least 1");
    if (pc.io_count_max > 0 && pc.io_gap_min < 1)
        inputError(path, header_line, "'IO gap' must be at least 1");
    if (pc.io_count_max > 0 && pc.io_dur_min < 1)
        inputError(path, header_line, "'IO duration' must be at least 1");
    return pc;
}

std::vector<ProcessClass> parseInput(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        fprintf(stderr, "input error: cannot open '%s'\n", path.c_str());
        exit(2);
    }

    enum { OUTSIDE, EXPECT_BRACE, INSIDE } state = OUTSIDE;
    std::vector<ProcessClass> classes;
    std::map<std::string, std::pair<int, std::string>> fields;
    int header_line = 0, lineno = 0;
    std::string raw;

    while (std::getline(in, raw)) {
        lineno++;
        std::string line = trim(raw.substr(0, raw.find('#')));
        if (line.empty()) continue;

        if (state == OUTSIDE) {
            std::string header = normalizeKey(line);
            if (header == "process class:" || header == "process class") {
                state = EXPECT_BRACE;
                header_line = lineno;
                fields.clear();
            } else if (header == "machine class:" || header == "task class:") {
                inputError(path, lineno, "'" + line + "' is a cloud-simulator stanza; use 'process class:'");
            } else {
                inputError(path, lineno, "expected 'process class:', got '" + line + "'");
            }
        } else if (state == EXPECT_BRACE) {
            if (line != "{")
                inputError(path, lineno, "expected '{' after 'process class:'");
            state = INSIDE;
        } else {
            if (line == "}") {
                classes.push_back(buildClass(path, header_line, fields));
                state = OUTSIDE;
                continue;
            }
            size_t colon = line.find(':');
            if (colon == std::string::npos)
                inputError(path, lineno, "expected 'Key: value', got '" + line + "'");
            std::string key = normalizeKey(line.substr(0, colon));
            if (fields.count(key))
                inputError(path, lineno, "field '" + key + "' appears twice in this class");
            fields[key] = {lineno, line.substr(colon + 1)};
        }
    }
    if (state != OUTSIDE)
        inputError(path, header_line, "process class is missing its closing '}'");
    if (classes.empty())
        inputError(path, lineno, "no process classes found");
    return classes;
}

void generateWorkload(const std::vector<ProcessClass>& classes, unsigned seed_offset) {
    class_types.clear();
    for (const ProcessClass& pc : classes) {
        int type_id = static_cast<int>(class_types.size());
        class_types.push_back(pc.type);

        std::mt19937 gen(pc.seed + seed_offset);
        std::exponential_distribution<double> gap_dist(1.0 / static_cast<double>(pc.inter_arrival));
        std::uniform_int_distribution<int> work_dist(pc.runtime_min, pc.runtime_max);
        std::uniform_int_distribution<int> io_count_dist(pc.io_count_min, pc.io_count_max);
        std::uniform_int_distribution<int> io_gap_dist(pc.io_gap_min, pc.io_gap_max);
        std::uniform_int_distribution<int> io_dur_dist(pc.io_dur_min, pc.io_dur_max);

        std::uint64_t t = pc.start_time;
        while (t <= pc.end_time) {
            for (int b = 0; b < pc.batch_size; b++) {
                int work = work_dist(gen);

                // Drop any I/O that would start after process finishes
                std::vector<IO> io;
                std::uint64_t io_total = 0;
                std::uint64_t io_start = 0;
                int io_num = io_count_dist(gen);
                for (int k = 0; k < io_num; k++) {
                    io_start += static_cast<std::uint64_t>(io_gap_dist(gen)) * TICK_US;
                    std::uint64_t duration = static_cast<std::uint64_t>(io_dur_dist(gen)) * TICK_US;
                    if (io_start >= static_cast<std::uint64_t>(work * TICK_US))
                        break;
                    io.push_back(IO{io_start, duration});
                    io_total += duration;
                }

                // PID must equal the index in process_list
                int pid = static_cast<int>(process_list.size());
                auto p = std::make_shared<Process>();
                p->io = io;
                p->arrival = t * TICK_US;
                p->last_core = -1;
                p->remaining = work * TICK_US; // shrinks as process runs
                p->PID = pid;
                p->cache_hot_duration = 0;
                p->cache_hot_until = 0;
                p->context_switched_hot = false;
                p->context_switched_from_io = false;
                p->quantum_credit = 10;
                p->yield_time = 0;
                p->type_id = type_id;
                p->work = work * TICK_US; // original runtime (distinct from remaining)
                p->io_count = static_cast<int>(io.size());
                p->io_total = io_total;
                process_list.push_back(p);

                event_queue.push(Event{EventType::ProcessArrival, t * TICK_US, pid});
            }
            // At least 1 tick between arrivals
            std::uint64_t gap = static_cast<std::uint64_t>(gap_dist(gen));
            t += std::max<std::uint64_t>(1, gap);
        }
    }
}

void setReportOptions(unsigned seed_offset, bool dump_processes) {
    report_seed_offset = seed_offset;
    report_dump = dump_processes;
}

static const char* modeName(SchedulerMode m) {
    switch (m) {
        case SchedulerMode::Preemptive:  return "preemptive";
        case SchedulerMode::Cooperative: return "cooperative";
        case SchedulerMode::Hybrid:      return "hybrid";
    }
    return "unknown";
}

struct Totals {
    std::uint64_t count = 0, completed = 0;
    double turnaround = 0, wait = 0, slowdown = 0;
    std::uint64_t max_turnaround = 0;

    void add(const Process& p) {
        count++;
        if (p.completion == 0) return;
        completed++;
        std::uint64_t ta = p.completion - p.arrival;
        std::uint64_t ideal = static_cast<std::uint64_t>(p.work) + p.io_total;
        turnaround += static_cast<double>(ta);
        wait += static_cast<double>(ta) - static_cast<double>(ideal);
        slowdown += static_cast<double>(ta) / static_cast<double>(ideal);
        max_turnaround = std::max(max_turnaround, ta);
    }
    void print(const char* prefix) const {
        double n = completed ? static_cast<double>(completed) : 1.0;
        printf("%s processes=%llu completed=%llu avg_turnaround=%.2f max_turnaround=%llu "
               "avg_wait=%.2f avg_slowdown=%.3f",
               prefix, (unsigned long long)count, (unsigned long long)completed,
               turnaround / n, (unsigned long long)max_turnaround, wait / n, slowdown / n);
    }
};

void printReport(bool success) {
    Totals all;
    std::vector<Totals> by_type(class_types.size());
    for (const auto& p : process_list) {
        all.add(*p);
        by_type[p->type_id].add(*p);
    }

    printf("RESULT mode=%s seed_offset=%u status=%s end_time=%llu context_switches=%llu ",
           modeName(sched_mode), report_seed_offset, success ? "ok" : "incomplete",
           (unsigned long long)real_time, (unsigned long long)context_switches);
    all.print("");
    // print total energy in Joules
    printf(" energy=%.6f\n", total_energy_nj / 1e9);
    printf("\n");

    std::map<std::string, Totals> merged;
    for (size_t i = 0; i < class_types.size(); i++) {
        Totals& m = merged[class_types[i]];
        const Totals& t = by_type[i];
        m.count += t.count; m.completed += t.completed;
        m.turnaround += t.turnaround; m.wait += t.wait; m.slowdown += t.slowdown;
        m.max_turnaround = std::max(m.max_turnaround, t.max_turnaround);
    }
    for (const auto& kv : merged) {
        std::string prefix = "TYPE type=" + kv.first;
        kv.second.print(prefix.c_str());
        printf("\n");
    }

    if (report_dump) {
        for (const auto& p : process_list)
            printf("PROC pid=%d type=%s arrival=%llu completion=%llu work=%d io_count=%d io_total=%llu\n",
                   p->PID, class_types[p->type_id].c_str(), (unsigned long long)p->arrival,
                   (unsigned long long)p->completion, p->work, p->io_count,
                   (unsigned long long)p->io_total);
    }
    fflush(stdout);
}
