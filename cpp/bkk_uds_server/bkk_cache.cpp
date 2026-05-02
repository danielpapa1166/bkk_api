#include "bkk_cache.hpp"


UdsCache::UdsCache(cache_config_t * config) {
  printf("Sizeof cache_entry_t: %lu bytes\n", sizeof(cache_entry_t));

  if(config == nullptr) {
    return;
  }

  cache_freshness_seconds = config->freshness_seconds;
  cache_staleness_seconds = config->staleness_seconds;
  max_cache_size = config->max_cache_size;
}


void UdsCache::put_element(
    const std::string & stop_id, const cache_entry_t & cache_entry) {    
  {
    std::lock_guard<std::mutex> lock(cache_mutex);
    cache[stop_id] = cache_entry;
  }

  // Clean up stale entries after adding a new one
  remove_stale_entries();

  printf("Cache size in bytes: %lu\n", cache.size() * sizeof(cache_entry_t));
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

  if (now - entry.timestamp < std::chrono::seconds(cache_freshness_seconds)) {
    printf("Fresh cache hit: now: %lld, timestamp: %lld\n",
      (long long)std::chrono::duration_cast<std::chrono::seconds>(
      now.time_since_epoch()).count(),
      (long long)std::chrono::duration_cast<std::chrono::seconds>(
      entry.timestamp.time_since_epoch()).count());

    *entry_out = entry;
    return CACHE_HIT_FRESH;
  } 
  else if (now - entry.timestamp < std::chrono::seconds(cache_staleness_seconds)) {

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


void UdsCache::remove_stale_entries() {
  std::lock_guard<std::mutex> lock(cache_mutex);
  const auto now = std::chrono::steady_clock::now();
  for (auto it = cache.begin(); it != cache.end(); ) {
    if (now - it->second.timestamp >= std::chrono::seconds(cache_staleness_seconds)) {
      it = cache.erase(it);
    } else {
      printf("Cache entry: stop_id: %s, timestamp: %lld, now: %lld\n", 
        it->first.c_str(), 
        (long long)std::chrono::duration_cast<std::chrono::seconds>(
          it->second.timestamp.time_since_epoch()).count(),
        (long long)std::chrono::duration_cast<std::chrono::seconds>(
          now.time_since_epoch()).count());
      ++it;
    }
  }
}