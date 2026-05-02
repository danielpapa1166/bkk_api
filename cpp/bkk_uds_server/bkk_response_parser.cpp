#include "bkk_response_parser.hpp"
#include <nlohmann/json.hpp>
#include <cmath>
#include <cstring>
#include <iostream>
#include <ctime>
#include <sstream>
#include <iomanip>

using json = nlohmann::json;

// parse arrival response from BKK Server 

static std::string find_route_id_for_trip(
    const std::string& trip_id, 
    const json& references) {
    
    // trips reference contains trip details    
    try {
        if (references.contains("trips") && references["trips"].contains(trip_id)) {
            auto trip = references["trips"][trip_id];
            if (trip.contains("routeId")) {
                // we need the rout id to find the line name
                return trip["routeId"].get<std::string>();
            }
        }
    } 
    catch (const std::exception& e) {
        std::cerr << "Error finding route ID for trip: " << e.what() << std::endl;
    }
    return "";
}

static std::string find_route_name_for_route_id(
    const std::string& route_id, 
    const json& references) {
    
    // the routes reference contins route info 
    try {
        if (references.contains("routes") && references["routes"].contains(route_id)) {
            auto route = references["routes"][route_id];
            if (route.contains("shortName")) {
                // short name is the line name 
                return route["shortName"].get<std::string>();
            }
        }
    } 
    catch (const std::exception& e) {
        std::cerr << "Error finding route name for route ID: " << e.what() << std::endl;
    }
    return "";
}

std::vector<Arrival> parse_arrivals_response(const std::string& response_body) {
    std::vector<Arrival> arrivals;

    json response;
    try {
        response = json::parse(response_body);
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse response JSON: " << e.what() << std::endl;
        return arrivals;
    }
    
    // Check if response has required data structure
    if (!response.contains("data") || !response["data"].contains("entry")) {
        std::cout << "No schedule info for the given stop" << std::endl;
        return arrivals;
    }
    
    try {
        // Get API time in seconds (convert from milliseconds)
        double apiTime_s = response["currentTime"].get<long long>() / 1000.0;
        
        const auto& entry = response["data"]["entry"];

        // stop info: 
        const auto& stop_times = entry.contains("stopTimes") ? 
            entry["stopTimes"] : json::array();

        // references contains extra info about trip id and route details 
        const auto& references = response["data"].contains("references") ? 
            response["data"]["references"] : json::object();
        
        // Get trips reference
        const auto& trips = references.contains("trips") ? 
            references["trips"] : json::object();
        
        // Parse each stop time
        for (const auto& stop_time : stop_times) {
            try {
                // trip id of the current stop time: get line info 
                std::string trip_id = stop_time.contains("tripId") ? 
                    stop_time["tripId"].get<std::string>() : "";
                
                // find route information
                std::string route_id = find_route_id_for_trip(trip_id, references);
                std::string line = find_route_name_for_route_id(route_id, references);
                if (line.empty()) {
                    line = !route_id.empty() ? route_id : "n.a.";
                }
                
                // Get departure time 
                long long departure_time = 0;
                if (stop_time.contains("predictedArrivalTime") && 
                    stop_time["predictedArrivalTime"].is_number()) {
                    // try to get predicted: 
                    departure_time = stop_time["predictedArrivalTime"].get<long long>();
                } else if (stop_time.contains("arrivalTime") && 
                           stop_time["arrivalTime"].is_number()) {

                    // if not available get scheduled (always available) 
                    departure_time = stop_time["arrivalTime"].get<long long>();
                }
                
                // Calculate minutes until departure
                int departs_in_min = (int)std::round((departure_time - apiTime_s) / 60.0);
                
                // Format departure time as HH:MM
                std::time_t time_t_val = (std::time_t)departure_time;
                std::tm* timeinfo = std::localtime(&time_t_val);
                std::stringstream ss;
                ss << std::put_time(timeinfo, "%H:%M");
                std::string departure_time_str = ss.str();
                
                // Get destination
                std::string destination = "?";
                if (trips.contains(trip_id) && trips[trip_id].contains("tripHeadsign")) {
                    destination = trips[trip_id]["tripHeadsign"].get<std::string>();
                }
                
                // Create and add arrival
                Arrival arrival {};
                strncpy(arrival.line_id, 
                    line.c_str(), sizeof(arrival.line_id) - 1);
                strncpy(arrival.destination, 
                    destination.c_str(), sizeof(arrival.destination) - 1);
                strncpy(arrival.departure_time, 
                    departure_time_str.c_str(), sizeof(arrival.departure_time) - 1);
                arrival.departs_in_min = departs_in_min;
                arrival.timestamp = departure_time;
                
                arrivals.push_back(arrival);
                
            } catch (const std::exception& e) {
                std::cerr << "Error parsing stop time: " << e.what() << std::endl;
                continue;
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse entry from response: " << e.what() << std::endl;
    }
    
    return arrivals;
}