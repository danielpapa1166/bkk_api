#include "bkk_api.hpp"
#include "bkk_api_types.h"
#include "bkk_response_parser.hpp"
#include <rbuflogd/logger.h>
#include <curl/curl.h>
#include <cstdlib>
#include <iostream>
#include <map>

namespace bkk_api {

constexpr const char* BASE_URL = "https://futar.bkk.hu/api/query/v1/ws/otp/api/where";
const char * CAT = "bkk_api";

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
  userp->append((char*)contents, size * nmemb);
  return size * nmemb;
}

bool is_successful_parse_status(ArrivalsParseStatus status) {
  return status == ArrivalsParseStatus::Success
      || status == ArrivalsParseStatus::SuccessWithWarnings;
}

bkk_api::ErrorCode make_request(
    const std::string& endpoint,
    const std::string& api_key,
    const std::map<std::string, std::string>& params,
    std::string* const response_out) {

  if (response_out == nullptr) {
    return bkk_api::ErrorCode::UnexpectedException;
  }

  if(api_key.empty()) {
    return bkk_api::ErrorCode::MissingApiKey;
  }

  std::string url = std::string(BASE_URL) + "/" + endpoint + "?";

  bool first = true;
  for (const auto& param : params) {
    if (!first) {
      url += "&";
    }
    url += param.first + "=" + param.second;
    first = false;
  }

  if (!first) {
    url += "&";
  }
  url += "key=" + api_key;

  response_out->clear();

  CURL* curl = curl_easy_init();
  if (!curl) {
    return bkk_api::ErrorCode::CurlInitFailed;
  }

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)10);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, response_out);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "BKKApiCpp/1.0");

  CURLcode res = curl_easy_perform(curl);
  long http_code = 0;
  if (res == CURLE_OK) {
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
  }
  curl_easy_cleanup(curl);

  if (res != CURLE_OK) {
    return bkk_api::ErrorCode::CurlPerformFailed;
  }
  if(http_code == 401) {
    return bkk_api::ErrorCode::InvalidApiKey;
  }

  if (http_code < 200 || http_code >= 300) {
    return bkk_api::ErrorCode::HttpError;
  }

  return bkk_api::ErrorCode::Ok;
}

bkk_api::ErrorCode get_arrivals_for_stop(
    const std::string& stop_id,
    const std::string& api_key,
    std::string* const response_out) {

  std::string formatted_stop_id = stop_id;
  if (formatted_stop_id.rfind("BKK_", 0) != 0) {
    formatted_stop_id = "BKK_" + formatted_stop_id;
  }

  const std::map<std::string, std::string> params = {
    {"stopId", formatted_stop_id},
    {"onlyDepartures", "1"},
    {"minutesBefore", "0"},
    {"minutesAfter", "30"}
  };

  return make_request(
      "arrivals-and-departures-for-stop.json", api_key, params, response_out);
}

} // namespace bkk_api

std::string bkk_api::get_env_var(const std::string& key) {
  const char* val = std::getenv(key.c_str());
  return (val == nullptr) ? std::string() : std::string(val);
}

bkk_api::ErrorCode bkk_api::get_arrivals_for_station(
    const std::string& stop_id,
    const std::string& api_key,
    std::vector<Arrival>* const output_arrivals) {

  if (output_arrivals == nullptr) {
    return bkk_api::ErrorCode::UnexpectedException;
  }

  output_arrivals->clear();

  try {
    std::string server_response;
    bkk_api::ErrorCode fetch_status = get_arrivals_for_stop(
        stop_id, api_key, &server_response);
    if (fetch_status != ErrorCode::Ok) {
      log_error(CAT, ("Failed to fetch arrivals for stop " + stop_id
                + ", error: " + error_code_to_string(fetch_status)).c_str());
      return fetch_status;
    }

    const ArrivalsParseStatus parse_status = parse_arrivals_response(
        server_response, output_arrivals);
    if (!is_successful_parse_status(parse_status)) {
      std::cerr << "Arrival parsing returned status code: "
                << static_cast<int>(parse_status) << std::endl;

      log_error(CAT, ("Failed to parse arrivals for stop " + stop_id
                + ", parse status: " + parse_status_to_string(parse_status)).c_str());
      return bkk_api::ErrorCode::ArrivalsParseFailed;
    }

    return ErrorCode::Ok;
  } catch (const std::exception& e) {
    printf("Error getting arrivals for station: %s\n", e.what());
    return ErrorCode::UnexpectedException;
  }
}


void bkk_api::display_arrivals(const std::vector<Arrival>& arrivals) {
  if (arrivals.empty()) {
    std::cout << "No arrivals found for the selected station." << std::endl;
    return;
  }

  std::cout << "Upcoming arrivals:" << std::endl;
  for (size_t i = 0; i < arrivals.size(); i++) {
    std::cout << "Line: " << arrivals[i].line_id
      << ", Destination: " << arrivals[i].destination
      << ", Departure Time: " << arrivals[i].departure_time
      << ", Departs in: " << arrivals[i].departs_in_min << " min"
      << std::endl;
  }
}