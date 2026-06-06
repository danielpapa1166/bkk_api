#include "bkk_response_parser.hpp"
#include "bkk_api_arrival.h"
#include "cJSON/cJSON.h"
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

  cJSON* trips = cJSON_GetObjectItemCaseSensitive(
    references, "trips");
  if (!cJSON_IsObject(trips)) {
    return "";
  }

  cJSON* trip = cJSON_GetObjectItemCaseSensitive(
    trips, trip_id.c_str());
  if (!cJSON_IsObject(trip)) {
    return "";
  }

  cJSON* route_id = cJSON_GetObjectItemCaseSensitive(
    trip, "routeId");
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


static vehicle_type_t find_type_for_route_id(
    const std::string & route_id, 
    const cJSON * references) {
  
  if ((references == nullptr) || route_id.empty()) {
    return VEHICLE_TYPE_UNKNOWN;
  }

    cJSON* routes = cJSON_GetObjectItemCaseSensitive(references, "routes");
  if (!cJSON_IsObject(routes)) {
    return VEHICLE_TYPE_UNKNOWN;
  }

  cJSON* route = cJSON_GetObjectItemCaseSensitive(routes, route_id.c_str());
  if (!cJSON_IsObject(route)) {
    return VEHICLE_TYPE_UNKNOWN;
  }

  cJSON* type_item = cJSON_GetObjectItemCaseSensitive(route, "type");
  if (cJSON_IsString(type_item) && (type_item->valuestring != nullptr)) {
    const char* type_str = type_item->valuestring;
    if      (strcmp(type_str, "BUS")              == 0) return VEHICLE_TYPE_BUS;
    else if (strcmp(type_str, "TRAM")             == 0) return VEHICLE_TYPE_TRAM;
    else if (strcmp(type_str, "TROLLEYBUS")       == 0) return VEHICLE_TYPE_TROLLEYBUS;
    else if (strcmp(type_str, "SUBWAY")           == 0) return VEHICLE_TYPE_METRO;
    else if (strcmp(type_str, "SUBURBAN_RAILWAY") == 0) return VEHICLE_TYPE_SUBURB_RAIL;
    else if (strcmp(type_str, "RAIL")             == 0) return VEHICLE_TYPE_RAIL;
    else if (strcmp(type_str, "FERRY")            == 0) return VEHICLE_TYPE_FERRY;
    else if (strcmp(type_str, "CABLE_CAR")        == 0) return VEHICLE_TYPE_CABLE_CAR;
    else if (strcmp(type_str, "FUNICULAR")        == 0) return VEHICLE_TYPE_FUNICULAR;
    else if (strcmp(type_str, "GONDOLA")          == 0) return VEHICLE_TYPE_GONDOLA;
    else if (strcmp(type_str, "COACH")            == 0) return VEHICLE_TYPE_COACH;
    else if (strcmp(type_str, "BICYCLE")          == 0) return VEHICLE_TYPE_BICYCLE;
    else if (strcmp(type_str, "CAR")              == 0) return VEHICLE_TYPE_CAR;
    else if (strcmp(type_str, "WALK")             == 0) return VEHICLE_TYPE_WALK;
    else if (strcmp(type_str, "LOCAL")            == 0) return VEHICLE_TYPE_LOCAL;
    else if (strcmp(type_str, "TRANSIT")          == 0) return VEHICLE_TYPE_TRANSIT;
  }

  return VEHICLE_TYPE_UNKNOWN;
}


ArrivalsParseStatus parse_arrivals_response(
  const std::string& response_body,
  std::vector<Arrival>* const output_arrivals) {

  if (output_arrivals == nullptr) {
    return ArrivalsParseStatus::NoValidStopTimes;
  }

  output_arrivals->clear();
  int skipped_stop_times = 0;

  cJSON* response = cJSON_Parse(response_body.c_str());
  if (response == nullptr) {
    const char* error_ptr = cJSON_GetErrorPtr();
    std::cerr << "Failed to parse response JSON with cJSON";
    if (error_ptr != nullptr) {
      std::cerr << " near: " << error_ptr;
    }
    std::cerr << std::endl;
    return ArrivalsParseStatus::InvalidJson;
  }

  if (!cJSON_IsObject(response)) {
    std::cerr << "Response root is not a JSON object" << std::endl;
    cJSON_Delete(response);
    return ArrivalsParseStatus::RootNotObject;
  }

  cJSON* data = cJSON_GetObjectItemCaseSensitive(response, "data");
  if (!cJSON_IsObject(data)) {
    cJSON_Delete(response);
    return ArrivalsParseStatus::MissingDataObject;
  }

  cJSON* entry = cJSON_IsObject(data)
    ? cJSON_GetObjectItemCaseSensitive(data, "entry")
    : nullptr;
  if (!cJSON_IsObject(entry)) {
    std::cout << "No schedule info for the given stop" << std::endl;
    cJSON_Delete(response);
    return ArrivalsParseStatus::MissingEntryObject;
  }

  // Get API time in seconds (convert from milliseconds)
  cJSON* current_time_item = cJSON_GetObjectItemCaseSensitive(response, "currentTime");
  double apiTime_s = cJSON_IsNumber(current_time_item)
    ? cJSON_GetNumberValue(current_time_item) / 1000.0
    : 0.0;

  cJSON* stop_times = cJSON_GetObjectItemCaseSensitive(entry, "stopTimes");
  if (!cJSON_IsArray(stop_times)) {
    cJSON_Delete(response);
    return ArrivalsParseStatus::StopTimesNotArray;
  }

  cJSON* references = cJSON_IsObject(data)
    ? cJSON_GetObjectItemCaseSensitive(data, "references")
    : nullptr;
  cJSON* trips = cJSON_IsObject(references)
    ? cJSON_GetObjectItemCaseSensitive(references, "trips")
    : nullptr;

  const int stop_time_count = cJSON_GetArraySize(stop_times);
  if (stop_time_count == 0) {
    cJSON_Delete(response);
    return ArrivalsParseStatus::EmptyStopTimes;
  }

  for (int i = 0; i < stop_time_count; ++i) {
    cJSON* stop_time = cJSON_GetArrayItem(stop_times, i);
    if (!cJSON_IsObject(stop_time)) {
      skipped_stop_times++;
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
    vehicle_type_t vehicle_type = find_type_for_route_id(route_id, references);
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
    else {
      skipped_stop_times++;
      continue;
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
    arrival.vehicle_type = vehicle_type;
    arrival.timestamp = departure_time;

    output_arrivals->push_back(arrival);
  }

  cJSON_Delete(response);

  if (output_arrivals->empty()) {
    return ArrivalsParseStatus::NoValidStopTimes;
  }
  if (skipped_stop_times > 0) {
    return ArrivalsParseStatus::SuccessWithWarnings;
  }

  return ArrivalsParseStatus::Success;
}