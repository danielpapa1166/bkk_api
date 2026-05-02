#ifndef BKK_API_HPP
#define BKK_API_HPP

#include "bkk_api_arrival.h"
#include <string>
#include <vector>

// TODO: review the API: is it thread safe? 

namespace bkk_api {

std::string get_env_var(const std::string& key);
std::vector<Arrival> get_arrivals_for_station(
    const std::string& stop_id, const std::string& api_key);
void display_arrivals(const std::vector<Arrival>& arrivals);

} // namespace bkk_api

#endif // BKK_API_HPP