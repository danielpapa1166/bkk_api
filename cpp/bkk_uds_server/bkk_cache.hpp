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

typedef struct {
  int freshness_seconds;
  int staleness_seconds;
  int max_cache_size;
} cache_config_t;


struct UdsCache {
  UdsCache(cache_config_t * config = nullptr); 

  void put_element(const std::string & stop_id, const cache_entry_t & cache_entry); 
  cache_state_t get_element(const std::string & stop_id, cache_entry_t * entry_out);

private:
  void remove_stale_entries();

  mutable std::mutex cache_mutex;
  std::unordered_map<std::string, cache_entry_t> cache;

  static constexpr int CACHE_FRESHNESS_SECONDS = 10;
  static constexpr int CACHE_STALENESS_SECONDS = 20;
  static constexpr int MAX_CACHE_SIZE = 100; 

  int cache_freshness_seconds = CACHE_FRESHNESS_SECONDS;
  int cache_staleness_seconds = CACHE_STALENESS_SECONDS;
  int max_cache_size = MAX_CACHE_SIZE;

}; 


#endif // BKK_CACHE_HPP