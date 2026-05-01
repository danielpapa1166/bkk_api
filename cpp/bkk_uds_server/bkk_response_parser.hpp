#pragma once
#include <vector>
#include <nlohmann/json.hpp>
#include "bkk_api_arrival.h"

using json = nlohmann::json;

std::vector<Arrival> parse_arrivals_response(const json& response);