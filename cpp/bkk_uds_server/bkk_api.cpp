#include "bkk_api.hpp"
#include "bkk_response_parser.hpp"
#include <curl/curl.h>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <map>

namespace {

constexpr const char* BASE_URL = "https://futar.bkk.hu/api/query/v1/ws/otp/api/where";

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
#ifdef BKK_API_VERBOSE_ON
  printf("Curl write callback called with size: %ld bytes\n", (size * nmemb));
#endif
  userp->append((char*)contents, size * nmemb);
  return size * nmemb;
}

std::string make_request(
    const std::string& endpoint,
    const std::string& api_key,
    const std::map<std::string, std::string>& params) {

  if(api_key.empty()) {
    throw std::runtime_error("API key is required");
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

  std::string read_buffer;

  const auto curl_init_start = std::chrono::steady_clock::now();
  CURL* curl = curl_easy_init();
  const auto curl_init_end = std::chrono::steady_clock::now();
  const auto curl_init_us = std::chrono::duration_cast<std::chrono::microseconds>(
      curl_init_end - curl_init_start).count();
  printf("curl_easy_init time: %lld us\n", (long long)curl_init_us);

  if (!curl) {
    throw std::runtime_error("Failed to initialize CURL");
  }

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)10);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &read_buffer);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "BKKApiCpp/1.0");

#ifdef BKK_API_VERBOSE_ON
  std::cout << "Making request to: " << url << std::endl;
#endif

  CURLcode res = curl_easy_perform(curl);
  long http_code = 0;
  if (res == CURLE_OK) {
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
  }
  curl_easy_cleanup(curl);

  if (res != CURLE_OK) {
    throw std::runtime_error("API request failed: " + std::string(curl_easy_strerror(res)));
  }
  if (http_code < 200 || http_code >= 300) {
    throw std::runtime_error("HTTP Error: " + std::to_string(http_code));
  }

  return read_buffer;
}

std::string get_arrivals_for_stop(const std::string& stop_id, const std::string& api_key) {
  try {
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
    return make_request("arrivals-and-departures-for-stop.json", api_key, params);
  } catch (const std::exception& e) {
    std::cerr << "Failed to fetch arrivals for stop " << stop_id << ": " << e.what() << std::endl;
    return "{}";
  }
}

} // namespace

std::string bkk_api::get_env_var(const std::string& key) {
  const char* val = std::getenv(key.c_str());
  return (val == nullptr) ? std::string() : std::string(val);
}

std::vector<Arrival> bkk_api::get_arrivals_for_station(
    const std::string& stop_id, const std::string& api_key) {
  try {
    const std::string server_response = get_arrivals_for_stop(stop_id, api_key);
    return parse_arrivals_response(server_response);
  } catch (const std::exception& e) {
    printf("Error getting arrivals for station: %s\n", e.what());
    return {};
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