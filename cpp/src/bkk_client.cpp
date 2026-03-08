#include "bkk_client.hpp"
#include <iostream>
#include <sstream>

// this class is responsible for making API requests to the BKK server 
// and returning the raw JSON response 

// Callback for CURL to write response data
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    printf("Curl write callback called with size: %d bytes\n", (size * nmemb));
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

BKKClient::BKKClient(const std::string & api_key)
    : api_key(api_key), 
      curl(curl_easy_init(), &curl_easy_cleanup) {
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }
}

BKKClient::~BKKClient() {
    // CURL is automatically cleaned up via unique_ptr deleter
}

nlohmann::json BKKClient::_make_request(
    const std::string& endpoint, 
    const std::map<std::string, std::string>& params) {
    
    // Build URL with query parameters
    std::string url = std::string(BASE_URL) + "/" + endpoint + "?";
    
    bool first = true;
    for (const auto& param : params) {
        if (!first) {
            url += "&";
        }
        
        url += param.first + "=" + param.second;
        first = false;
    }
    
    if (!first)  {
        url += "&"; 
    }
    url += "key=" + api_key;

    
    std::string readBuffer;
    
    try {
        std::cout << "Making request to: " << url << std::endl;
        
        // set request options: 
        curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, (long)10);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &readBuffer);
        
        // Set User-Agent
        curl_easy_setopt(curl.get(), CURLOPT_USERAGENT, "BKKClientCpp/1.0");
        
        // Perform request
        CURLcode res = curl_easy_perform(curl.get());
        
        if (res != CURLE_OK) {
            std::string error = "API request failed: " + std::string(curl_easy_strerror(res));
            std::cerr << error << std::endl;
            throw std::runtime_error(error);
        }
        
        // Check HTTP response code
        long http_code = 0;
        curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &http_code);
        if (http_code < 200 || http_code >= 300) {
            std::string error = "HTTP Error: " + std::to_string(http_code);
            std::cerr << error << std::endl;
            throw std::runtime_error(error);
        }
        
        // Parse JSON response
        nlohmann::json json_response = nlohmann::json::parse(readBuffer);
        return json_response;
        
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
        throw;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        throw;
    }
}

nlohmann::json BKKClient::get_arrivals_for_stop(const std::string& stop_id) {
    try {
        // Format the stop ID with BKK prefix if not already present
        std::string formatted_stop_id = stop_id;
        if (stop_id.substr(0, 4) != "BKK_") {
            formatted_stop_id = "BKK_" + stop_id;
        }
        
        // assemble request params: 
        std::map<std::string, std::string> params = {
            {"stopId", formatted_stop_id},
            {"onlyDepartures", "1"},
            {"minutesBefore", "0"},
            {"minutesAfter", "30"}
        };
        
        return _make_request("arrivals-and-departures-for-stop.json", params);
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to fetch arrivals for stop " << stop_id << ": " << e.what() << std::endl;
        return nlohmann::json::object();
    }
}