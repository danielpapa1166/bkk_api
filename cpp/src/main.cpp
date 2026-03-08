#include <iostream>
#include <string>
#include <cstdlib>
#include "bkk_api.hpp"

// this is a test application for the BKK API 
// later to be integrated into a larger proejct

BkkApi api;


int main(int argc, char* argv[]) {
    std::string station_id;
    if (argc > 1) {
        // get station name from command line arguments
        station_id = argv[1];

        if(station_id.empty()) {
            printf("Got empty station ID. \n"); 
            return 0; 
        }
    }
    else {
        printf("Got no station ID. \n"); 
        return 0; 
        // todo: implement proper selection method 
    }
    
    
    try {
        std::vector<Arrival> arrivals = api.get_arrivals_for_station(station_id);
        api.display_arrivals(arrivals);
    } catch (const std::exception& e) {
        std::cerr << "Error fetching or displaying arrivals: " << e.what() << std::endl;
    }

}