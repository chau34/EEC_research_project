#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct ProcessClass {
    std::string type = "UNSPECIFIED";   // CPU, IO, INTERACTIVE, ...
    std::uint64_t start_time = 0;
    std::uint64_t end_time = 0;
    std::uint64_t inter_arrival = 1;
    int batch_size = 1;
    int runtime_min = 0, runtime_max = 0;
    int io_count_min = 0, io_count_max = 0;
    int io_gap_min = 1, io_gap_max = 1;
    int io_dur_min = 1, io_dur_max = 1;
    unsigned seed = 0;
    int line = 0;
};

// Parse input file
std::vector<ProcessClass> parseInput(const std::string& path);

void generateWorkload(const std::vector<ProcessClass>& classes, unsigned seed_offset);

void setReportOptions(unsigned seed_offset, bool dump_processes);

void printReport(bool success);
