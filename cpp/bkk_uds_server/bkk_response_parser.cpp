#include "bkk_response_parser.hpp"
#include "cJSON.h"
#include <cmath>
#include <cstring>
#include <iostream>
#include <ctime>
#include <sstream>
#include <iomanip>

// parse arrival response from BKK Server 

static std::string find_route_id_for_trip(
  const std::string& trip_id, 
  const cJSON* references) {
  // trips reference contains trip details
  if (references == nullptr) {
    return "";
  }

  cJSON* trips = cJSON_GetObjectItemCaseSensitive(references, "trips");
  if (!cJSON_IsObject(trips)) {
    return "";
  }

  cJSON* trip = cJSON_GetObjectItemCaseSensitive(trips, trip_id.c_str());
  if (!cJSON_IsObject(trip)) {
    return "";
  }

  cJSON* route_id = cJSON_GetObjectItemCaseSensitive(trip, "routeId");
  if (cJSON_IsString(route_id) && (route_id->valuestring != nullptr)) {
    return route_id->valuestring;
  }

  return "";
}

static std::string find_route_name_for_route_id(
  const std::string& route_id, 
  const cJSON* references) {
  // the routes reference contains route info
  if ((references == nullptr) || route_id.empty()) {
    return "";
  }

  cJSON* routes = cJSON_GetObjectItemCaseSensitive(references, "routes");
  if (!cJSON_IsObject(routes)) {
    return "";
  }

  cJSON* route = cJSON_GetObjectItemCaseSensitive(routes, route_id.c_str());
  if (!cJSON_IsObject(route)) {
    return "";
  }

  cJSON* short_name = cJSON_GetObjectItemCaseSensitive(route, "shortName");
  if (cJSON_IsString(short_name) && (short_name->valuestring != nullptr)) {
    return short_name->valuestring;
  }

  return "";
}

std::vector<Arrival> parse_arrivals_response(const std::string& response_body) {
  std::vector<Arrival> arrivals;

  cJSON* response = cJSON_Parse(response_body.c_str());
  if (response == nullptr) {
    const char* error_ptr = cJSON_GetErrorPtr();
    std::cerr << "Failed to parse response JSON with cJSON";
    if (error_ptr != nullptr) {
      std::cerr << " near: " << error_ptr;
    }
    std::cerr << std::endl;
    return arrivals;
  }

  if (!cJSON_IsObject(response)) {
    std::cerr << "Response root is not a JSON object" << std::endl;
    cJSON_Delete(response);
    return arrivals;
  }

  cJSON* data = cJSON_GetObjectItemCaseSensitive(response, "data");
  cJSON* entry = cJSON_IsObject(data)
    ? cJSON_GetObjectItemCaseSensitive(data, "entry")
    : nullptr;
  if (!cJSON_IsObject(entry)) {
    std::cout << "No schedule info for the given stop" << std::endl;
    cJSON_Delete(response);
    return arrivals;
  }

  // Get API time in seconds (convert from milliseconds)
  cJSON* current_time_item = cJSON_GetObjectItemCaseSensitive(response, "currentTime");
  double apiTime_s = cJSON_IsNumber(current_time_item)
    ? cJSON_GetNumberValue(current_time_item) / 1000.0
    : 0.0;

  cJSON* stop_times = cJSON_GetObjectItemCaseSensitive(entry, "stopTimes");
  if (!cJSON_IsArray(stop_times)) {
    cJSON_Delete(response);
    return arrivals;
  }

  cJSON* references = cJSON_IsObject(data)
    ? cJSON_GetObjectItemCaseSensitive(data, "references")
    : nullptr;
  cJSON* trips = cJSON_IsObject(references)
    ? cJSON_GetObjectItemCaseSensitive(references, "trips")
    : nullptr;

  const int stop_time_count = cJSON_GetArraySize(stop_times);
  for (int i = 0; i < stop_time_count; ++i) {
    cJSON* stop_time = cJSON_GetArrayItem(stop_times, i);
    if (!cJSON_IsObject(stop_time)) {
      continue;
    }

    // trip id of the current stop time: get line info
    std::string trip_id;
    cJSON* trip_id_item = cJSON_GetObjectItemCaseSensitive(stop_time, "tripId");
    if (cJSON_IsString(trip_id_item) && (trip_id_item->valuestring != nullptr)) {
      trip_id = trip_id_item->valuestring;
    }

    // find route information
    std::string route_id = find_route_id_for_trip(trip_id, references);
    std::string line = find_route_name_for_route_id(route_id, references);
    if (line.empty()) {
      line = !route_id.empty() ? route_id : "n.a.";
    }

    // Get departure time
    long long departure_time = 0;
    cJSON* predicted_arrival = cJSON_GetObjectItemCaseSensitive(stop_time, "predictedArrivalTime");
    cJSON* arrival_time = cJSON_GetObjectItemCaseSensitive(stop_time, "arrivalTime");
    if (cJSON_IsNumber(predicted_arrival)) {
      departure_time = static_cast<long long>(cJSON_GetNumberValue(predicted_arrival));
    } 
    else if (cJSON_IsNumber(arrival_time)) {
      departure_time = static_cast<long long>(cJSON_GetNumberValue(arrival_time));
    }

    // Calculate minutes until departure
    const int departs_in_min = static_cast<int>(
      std::round((static_cast<double>(departure_time) - apiTime_s) / 60.0));

    // Format departure time as HH:MM
    const std::time_t time_t_val = static_cast<std::time_t>(departure_time);
    std::tm* timeinfo = std::localtime(&time_t_val);
    std::stringstream ss;
    if (timeinfo != nullptr) {
      ss << std::put_time(timeinfo, "%H:%M");
    } 
    else {
      ss << "00:00";
    }
    const std::string departure_time_str = ss.str();

    // Get destination
    std::string destination = "?";
    if (cJSON_IsObject(trips) && !trip_id.empty()) {
      cJSON* trip = cJSON_GetObjectItemCaseSensitive(trips, trip_id.c_str());
      if (cJSON_IsObject(trip)) {
        cJSON* headsign = cJSON_GetObjectItemCaseSensitive(trip, "tripHeadsign");
        if (cJSON_IsString(headsign) && (headsign->valuestring != nullptr)) {
          destination = headsign->valuestring;
        }
      }
    }

    // Create and add arrival
    Arrival arrival {};
    strncpy(arrival.line_id,
      line.c_str(), sizeof(arrival.line_id) - 1);
    arrival.line_id[sizeof(arrival.line_id) - 1] = '\0';
    strncpy(arrival.destination,
      destination.c_str(), sizeof(arrival.destination) - 1);
    arrival.destination[sizeof(arrival.destination) - 1] = '\0';
    strncpy(arrival.departure_time,
      departure_time_str.c_str(), sizeof(arrival.departure_time) - 1);
    arrival.departure_time[sizeof(arrival.departure_time) - 1] = '\0';
    arrival.departs_in_min = departs_in_min;
    arrival.timestamp = departure_time;

    arrivals.push_back(arrival);
  }

  cJSON_Delete(response);
  return arrivals;
}