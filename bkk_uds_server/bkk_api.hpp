#ifndef BKK_API_HPP
#define BKK_API_HPP

#include "bkk_api_arrival.h"
#include "bkk_api_types.h"
#include <string>
#include <vector>

namespace bkk_api {


std::string get_env_var(const std::string& key);
bkk_api_status_t get_arrivals_for_station(
    const std::string& stop_id,
    const std::string& api_key,
    std::vector<Arrival>* const output_arrivals);
void display_arrivals(const std::vector<Arrival>& arrivals);

} // namespace bkk_api

#endif // BKK_API_HPP