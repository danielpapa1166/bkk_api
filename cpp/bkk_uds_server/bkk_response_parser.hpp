#pragma once
#include <vector>
#include <string>
#include "bkk_api_arrival.h"

std::vector<Arrival> parse_arrivals_response(const std::string& response_body);