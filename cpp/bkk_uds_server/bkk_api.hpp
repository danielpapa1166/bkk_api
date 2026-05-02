#ifndef BKK_API_HPP
#define BKK_API_HPP

#include "bkk_api_arrival.h"
#include <curl/curl.h>
#include <memory>
#include <string>
#include <vector>
#include <map>

// TODO: review the API: is it thread safe? 

class BkkApi {

private: 
    std::string _api_key;
    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> _curl;

    static constexpr const char * BASE_URL = "https://futar.bkk.hu/api/query/v1/ws/otp/api/where";

    std::string make_request(
        const std::string& endpoint,
        const std::map<std::string, std::string>& params);
    std::string get_arrivals_for_stop(const std::string& stop_id);

public:
    BkkApi();
    std::vector<Arrival> get_arrivals_for_station(const std::string& stop_id);
    void display_arrivals(const std::vector<Arrival>& arrivals) const;


}; 
#endif // BKK_API_HPP