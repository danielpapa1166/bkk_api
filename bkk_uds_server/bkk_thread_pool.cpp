#include "bkk_thread_pool.hpp"
#include <cstdio>

ThreadPool::ThreadPool(size_t num_threads) {
  for (size_t i = 0; i < num_threads; ++i) {
    workers.emplace_back(&ThreadPool::thread_function, this);
  }
}


void ThreadPool::thread_function() {
  int foo = 42;
  while(1) {
    foo ++; 
    (void) foo; 
  }
}

ThreadPool::~ThreadPool() {
  for (std::thread & worker : workers) {
    if(worker.joinable()) {
      worker.detach();
    }
  }
}