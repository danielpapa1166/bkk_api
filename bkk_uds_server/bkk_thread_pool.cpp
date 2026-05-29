#include "bkk_thread_pool.hpp"
#include <cstdio>

ThreadPool::ThreadPool(size_t num_threads) {
  for (size_t i = 0; i < num_threads; ++i) {
    workers.emplace_back(&ThreadPool::thread_function, this);
  }
}


bool ThreadPool::submit(task_t task) {
  std::lock_guard<std::mutex> lock(queue_mutex);
  tasks.push(task);
  return true;
}


void ThreadPool::thread_function() {
  int foo = 42;
  while(1) {
    foo ++; 
    (void) foo; 

    if(tasks.empty()) {
      std::this_thread::yield();
      continue;
    }
    else {
      task_t task = tasks.front();
      { 
        std::lock_guard<std::mutex> lock(queue_mutex);
        if(!tasks.empty()) {
          task = tasks.front();
          tasks.pop();
        }
        else {
          continue;
        }
      }
      task();
    }
  }
}

ThreadPool::~ThreadPool() {
  for (std::thread & worker : workers) {
    if(worker.joinable()) {
      worker.detach();
    }
  }
}