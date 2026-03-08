#pragma once
#include <string>

struct Arrival {
    std::string line;
    std::string destination;
    std::string departure_time;
    int departs_in_min;
    long long timestamp;
};