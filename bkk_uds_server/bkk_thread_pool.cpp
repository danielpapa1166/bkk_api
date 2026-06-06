#include "bkk_thread_pool.hpp"
#include "bkk_api.hpp"
#include "bkk_uds_protocol.h"
#include <rbuflogd/logger.h>
#include <unistd.h>
#include <sys/socket.h>
#include <vector>
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <chrono>

static std::atomic<int> s_next_thread_id{0};
static thread_local int tl_thread_id = -1;

ThreadPool::ThreadPool(size_t num_threads) {
  for (size_t i = 0; i < num_threads; ++i) {
    workers.emplace_back(&ThreadPool::thread_function, this);
  }

  log_info("Init", 
    ("Thread pool initialized with " 
    + std::to_string(num_threads) + " threads").c_str());
}


bool ThreadPool::submit(int client_fd, UdsCache & cache) {
  {
    std::lock_guard<std::mutex> lock(queue_mutex);
    task_queue.push({
      client_fd, 
      cache
    });
  }
  cv.notify_one();
  return true;
}


void ThreadPool::thread_function() {
  tl_thread_id = s_next_thread_id.fetch_add(1);
  while (true) {
    {
      std::unique_lock<std::mutex> lock(queue_mutex);
      cv.wait(lock, [this]() { return stop || !task_queue.empty(); });
      if (stop && task_queue.empty()) {
        return;
      }
      thread_task_t task = task_queue.front();
      task_queue.pop();
      lock.unlock();

      // handle the client request after unlock: 
      handle_client(task);
    }
  }
}

void ThreadPool::handle_client(thread_task_t task) {
  int client_fd = task.client_fd;
  UdsCache & cache = task.cache;

  const auto t_start = std::chrono::steady_clock::now();
  bkk_uds_request_t request {};
  ssize_t n = recv(client_fd, &request, sizeof(request), 0);
  if (n != sizeof(request)) {
    printf("Failed to receive data from client\n");
    log_error("Runtime", "Failed to receive data from client");
    close(client_fd);
    return;
  }

  // check cache first:
  std::vector<Arrival> arrivals; 
  bkk_api_status_t api_status; 
  cache_entry_t cache_entry;
  cache_state_t cache_state = cache.get_element(request.stop_id, &cache_entry);
  if (cache_state == CACHE_HIT_FRESH) {
    arrivals = cache_entry.arrivals;
    // no failed fetch entry gets into the cache
    api_status = bkk_api_status::Ok;
  } 
  else if (cache_state == CACHE_HIT_STALE) {
    // fetch fresh data in the background 
    std::thread([&cache, request, this](){
      std::vector<Arrival> fresh_arrivals;
      (void) fresh_fetch_and_update_cache(
        request,
        cache, 
        &fresh_arrivals);
    }).detach();

    arrivals = cache_entry.arrivals;

    // no failed fetch entry gets into the cache
    api_status = bkk_api_status::Ok;
  }
  else {
    api_status = fresh_fetch_and_update_cache(
      request, 
      cache, 
      &arrivals); 
  }

  int num_arrivals = std::min((int)arrivals.size(), BKK_UDS_MAX_ARRIVALS);


  bkk_uds_response_t response {};
  response.status = api_status;
  response.number_of_arrivals = num_arrivals;
  for (int i = 0; i < num_arrivals; i++) {
    response.arrivals[i] = arrivals[i];
  }


  ssize_t sent_size = send(client_fd, &response, sizeof(response), 0);
  if (sent_size != sizeof(response)) {
    log_error("Runtime", "Failed to send response to client");
  }

  close(client_fd);

  auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(
    std::chrono::steady_clock::now() - t_start).count();

  log_debug(("req@th" + std::to_string(tl_thread_id)).c_str(), 
      ("Handled request for stop_id: " 
      + std::string(request.stop_id) 
      + ", returned " + std::to_string(num_arrivals) + " arrivals," 
      + " cache_status: " + cache_state_to_string(cache_state)
      + " in " + std::to_string(duration_us) + " us").c_str());
}


bkk_api_status_t ThreadPool::fresh_fetch_and_update_cache(
    const bkk_uds_request_t & request, UdsCache & cache, 
    std::vector<Arrival> * arrivals_out) {

  std::vector<Arrival> fresh_arrivals;
  std::string api_key(request.api_key);
  std::string stop_id(request.stop_id);
  bkk_api_status_t fetch_status = bkk_api::get_arrivals_for_station(
    stop_id, api_key, &fresh_arrivals);
  if (fetch_status != bkk_api_status::Ok) {
    log_error("Fetch", ("Failed to fetch arrivals for stop ID: " + stop_id 
        + ", error: " + error_code_to_string(fetch_status)).c_str()); 
    return fetch_status; 
  }
  *arrivals_out = fresh_arrivals;

  cache_entry_t new_cache_entry {
    *arrivals_out, 
    std::chrono::steady_clock::now()
  };
  cache.put_element(stop_id, new_cache_entry);
  return bkk_api_status::Ok;
}


ThreadPool::~ThreadPool() {
  {
    std::lock_guard<std::mutex> lock(queue_mutex);
    stop = true;
  }
  cv.notify_all();
  for (std::thread & worker : workers) {
    if (worker.joinable()) {
      worker.join();
    }
  }
}