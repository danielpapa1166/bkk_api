#include "bkk_cache.hpp"

// todo: 
//   add per-stop in-flight fetch dedup (single flight) to prevent stampedes
//   limit cache size 

void UdsCache::put_element(
    const std::string & stop_id, const cache_entry_t & cache_entry) {    
  std::lock_guard<std::mutex> lock(cache_mutex);
  cache[stop_id] = cache_entry;
}


cache_state_t UdsCache::get_element(
    const std::string & stop_id, cache_entry_t * entry_out) {
  std::lock_guard<std::mutex> lock(cache_mutex);
  auto it = cache.find(stop_id);
  if(it == cache.end()) {
    return CACHE_MISS;
  }

  const cache_entry_t & entry = it->second;
  const auto now = std::chrono::steady_clock::now();

  if (now - entry.timestamp < std::chrono::seconds(CACHE_FRESHNESS_SECONDS)) {
    printf("Fresh cache hit: now: %lld, timestamp: %lld\n",
      (long long)std::chrono::duration_cast<std::chrono::seconds>(
      now.time_since_epoch()).count(),
      (long long)std::chrono::duration_cast<std::chrono::seconds>(
      entry.timestamp.time_since_epoch()).count());

    *entry_out = entry;
    return CACHE_HIT_FRESH;
  } 
  else if (now - entry.timestamp < std::chrono::seconds(CACHE_STALENESS_SECONDS)) {

    printf("Stale cache hit: now: %lld, timestamp: %lld\n",
      (long long)std::chrono::duration_cast<std::chrono::seconds>(
      now.time_since_epoch()).count(),
      (long long)std::chrono::duration_cast<std::chrono::seconds>(
      entry.timestamp.time_since_epoch()).count());

    *entry_out = entry;
    return CACHE_HIT_STALE;
  }
  return CACHE_MISS;
}