#ifndef BKK_CACHE_HPP
#define BKK_CACHE_HPP

#include "bkk_api_arrival.h"
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>

typedef struct {
  std::vector<Arrival> arrivals;
  std::chrono::steady_clock::time_point timestamp; 
} cache_entry_t;

typedef enum {
  CACHE_HIT_FRESH,
  CACHE_HIT_STALE, 
  CACHE_MISS, 
} cache_state_t;


struct UdsCache {
  void put_element(const std::string & stop_id, const cache_entry_t & cache_entry); 
  cache_state_t get_element(const std::string & stop_id, cache_entry_t * entry_out);

private:
  mutable std::mutex cache_mutex;
  std::unordered_map<std::string, cache_entry_t> cache;

  const int CACHE_FRESHNESS_SECONDS = 10;
  const int CACHE_STALENESS_SECONDS = 20;

}; 


#endif // BKK_CACHE_HPP