#ifndef BKK_API_HPP
#define BKK_API_HPP

#include "bkk_client.hpp"
#include "bkk_api_arrival.h"
#include <vector>
#include <string>

// TODO: review the API: is it thread safe? 

class BkkApi {

private: 
    std::unique_ptr<BKKClient> _client_ptr;

public:
    BkkApi();
    std::vector<Arrival> get_arrivals_for_station(const std::string& stop_id);
    void display_arrivals(const std::vector<Arrival>& arrivals) const;


}; 
#endif // BKK_API_HPP