#ifndef BKK_THREAD_POOL_HPP
#define BKK_THREAD_POOL_HPP

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <stdlib.h>
#include <thread>
#include <vector>
#include "bkk_api_types.h"
#include "bkk_cache.hpp"
#include "bkk_uds_protocol.h"

typedef struct {
  int client_fd;
  UdsCache & cache;
} thread_task_t;

class ThreadPool {

typedef void (* task_t)(int client_fd, UdsCache & cache);

public:
  ThreadPool(size_t num_threads);
  ~ThreadPool();
  bool submit(int client_fd, UdsCache & cache);

private:
  void thread_function(); 
  void handle_client(thread_task_t task); 
  bkk_api_status_t fresh_fetch_and_update_cache(
    const bkk_uds_request_t & request, UdsCache & cache, 
    std::vector<Arrival> * arrivals_out); 
  std::vector<std::thread> workers;
  std::queue<thread_task_t> task_queue;
  std::mutex queue_mutex;
  std::condition_variable cv;
  bool stop = false;
}; 


#endif // BKK_THREAD_POOL_HPP