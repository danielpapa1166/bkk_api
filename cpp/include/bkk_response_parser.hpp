#pragma once
#include <vector>
#include <nlohmann/json.hpp>
#include "arrival.hpp"

using json = nlohmann::json;

std::vector<Arrival> parse_arrivals_response(const json& response);