#include "bkk_api.hpp"
#include "bkk_client.hpp"
#include "bkk_response_parser.hpp"
#include "bkk_station_list.hpp"
#include <iostream>

// this is a wrapper class that for higher level API calls 
// to be compiled into a .so 


static std::string get_env_var(const std::string& key) {
    const char* val = std::getenv(key.c_str());
    return (val == nullptr) ? std::string() : std::string(val);
}

BkkApi::BkkApi(){
    std::string api_key = get_env_var("BKK_API_KEY");

    if (api_key.empty()) {
        throw std::runtime_error("BKK_API_KEY environment variable not set.");
    }
    
    // instanciate the BKK client with the API key: 
    _client_ptr = std::make_unique<BKKClient>(api_key);


}

std::vector<Arrival> BkkApi::get_arrivals_for_station(const std::string& stop_id) {
    try {
        // create a request to fetch arrivals for the given stop ID
        nlohmann::json server_response = _client_ptr->get_arrivals_for_stop(stop_id);
        // convert JSON resposne: 
        return parse_arrivals_response(server_response);
    } catch (const std::exception& e) {
        printf("Error getting arrivals for station: %s\n", e.what());
        return {};
    }
}

void BkkApi::display_arrivals(const std::vector<Arrival>& arrivals) const {
    if (arrivals.empty()) {
        std::cout << "No arrivals found for the selected station." << std::endl;
        return;
    }
    
    std::cout << "Upcoming arrivals:" << std::endl;
    for (int i = 0; i < arrivals.size(); i++) {
        std::cout << "Line: " << arrivals[i].line 
                  << ", Destination: " << arrivals[i].destination 
                  << ", Departure Time: " << arrivals[i].departure_time 
                  << ", Departs in: " << arrivals[i].departs_in_min << " min" 
                  << std::endl;
    }
}