#pragma once
#include <string>
#include <map>
#include <optional>
#include <memory>
#include <nlohmann/json.hpp>
#include <curl/curl.h>

using json = nlohmann::json;

class BKKClient {
private:
    std::string api_key;
    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl;
    
    static constexpr const char* BASE_URL = "https://futar.bkk.hu/api/query/v1/ws/otp/api/where";
    
    json _make_request(const std::string& endpoint, const std::map<std::string, std::string>& params);
    
public:
    BKKClient(const std::string & api_key);
    ~BKKClient();
    
    json get_arrivals_for_stop(const std::string& stop_id);
};