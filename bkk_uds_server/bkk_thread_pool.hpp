#ifndef BKK_THREAD_POOL_HPP
#define BKK_THREAD_POOL_HPP

#include <functional>
#include <mutex>
#include <queue>
#include <stdlib.h>
#include <thread>
#include <vector>

class ThreadPool {
public:
  ThreadPool(size_t num_threads);
  ~ThreadPool();
  bool submit(std::function<void()> task);

private:
  void thread_function(); 
  std::vector<std::thread> workers;
  std::queue<std::function<void()>> tasks;
  std::mutex queue_mutex;

}; 


#endif // BKK_THREAD_POOL_HPP