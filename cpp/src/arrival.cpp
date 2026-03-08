#pragma once
#include <string>
#include <optional>

struct Arrival {
    std::string line;
    std::string destination;
    std::optional<std::string> departure_time;
    std::optional<int> departs_in_min;
    std::optional<long long> timestamp;
    
    // Constructor
    Arrival(const std::string& l, const std::string& d,
            const std::optional<std::string>& dt = std::nullopt,
            const std::optional<int>& dim = std::nullopt,
            const std::optional<long long>& ts = std::nullopt)
        : line(l), destination(d), departure_time(dt), 
          departs_in_min(dim), timestamp(ts) {}
};