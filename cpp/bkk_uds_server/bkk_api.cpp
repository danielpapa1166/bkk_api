#include "bkk_api.hpp"
#include "bkk_response_parser.hpp"
#include <cstdlib>
#include <iostream>

static std::string get_env_var(const std::string& key) {
    const char* val = std::getenv(key.c_str());
    return (val == nullptr) ? std::string() : std::string(val);
}

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
#ifdef BKK_API_VERBOSE_ON
    printf("Curl write callback called with size: %ld bytes\n", (size * nmemb));
#endif
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

BkkApi::BkkApi()
    : _curl(curl_easy_init(), &curl_easy_cleanup) {
    _api_key = get_env_var("BKK_API_KEY");
    if (_api_key.empty()) {
        throw std::runtime_error("BKK_API_KEY environment variable not set.");
    }

    if (!_curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }
}

std::string BkkApi::make_request(
    const std::string& endpoint,
    const std::map<std::string, std::string>& params) {

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
    url += "key=" + _api_key;

    std::string read_buffer;

    curl_easy_setopt(_curl.get(), CURLOPT_URL, url.c_str());
    curl_easy_setopt(_curl.get(), CURLOPT_TIMEOUT, (long)10);
    curl_easy_setopt(_curl.get(), CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(_curl.get(), CURLOPT_WRITEDATA, &read_buffer);
    curl_easy_setopt(_curl.get(), CURLOPT_USERAGENT, "BKKApiCpp/1.0");

#ifdef BKK_API_VERBOSE_ON
    std::cout << "Making request to: " << url << std::endl;
#endif

    CURLcode res = curl_easy_perform(_curl.get());
    if (res != CURLE_OK) {
        throw std::runtime_error("API request failed: " + std::string(curl_easy_strerror(res)));
    }

    long http_code = 0;
    curl_easy_getinfo(_curl.get(), CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code < 200 || http_code >= 300) {
        throw std::runtime_error("HTTP Error: " + std::to_string(http_code));
    }

    return read_buffer;
}

std::string BkkApi::get_arrivals_for_stop(const std::string& stop_id) {
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
        return make_request("arrivals-and-departures-for-stop.json", params);
    } catch (const std::exception& e) {
        std::cerr << "Failed to fetch arrivals for stop " << stop_id << ": " << e.what() << std::endl;
        return "{}";
    }
}

std::vector<Arrival> BkkApi::get_arrivals_for_station(const std::string& stop_id) {
    try {
        const std::string server_response = get_arrivals_for_stop(stop_id);
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
    for (size_t i = 0; i < arrivals.size(); i++) {
        std::cout << "Line: " << arrivals[i].line_id
                  << ", Destination: " << arrivals[i].destination
                  << ", Departure Time: " << arrivals[i].departure_time
                  << ", Departs in: " << arrivals[i].departs_in_min << " min"
                  << std::endl;
    }
}