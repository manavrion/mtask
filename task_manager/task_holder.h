// task_manager.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <atomic>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <vector>
#include "task_manager.h"

// TODO: Reference additional headers your program requires here.

namespace task {

struct AddTaskAfterClose : std::exception {};

class TaskHolder {
  using task_ptr_t = std::unique_ptr<ITask>;
  using task_queue_t = std::vector<task_ptr_t>;
  using mutex_t = std::mutex;
  using auto_lock_t = std::lock_guard<mutex_t>;
  using thread_t = std::thread;
  using exception_ptr_t = std::exception_ptr;

  task_queue_t IncomingQueue;
  mutex_t IncomingQueueMutex;

  std::atomic<bool> IsClosure;
  exception_ptr_t ExceptionPtr;
  thread_t Thread;

  void Loop() {
    while (true) {
      task_queue_t ProcessingQueue;
      {
        auto_lock_t al(IncomingQueueMutex);
        std::swap(ProcessingQueue, IncomingQueue);
      }

      for (auto& task : ProcessingQueue) {
        try {
          task->Run();
        } catch (...) {
          ExceptionPtr = std::current_exception();
        }
      }

      bool sleep = false;
      {
        auto_lock_t al(IncomingQueueMutex);
        if (IncomingQueue.empty()) {
          if (IsClosure) break;
          sleep = true;
        }
      }
      if (sleep) std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
  }

 public:
  TaskHolder() : IsClosure(false), Thread(std::bind(&TaskHolder::Loop, this)) {}

  void AddTask(task_ptr_t task) {
    if (IsClosure) throw AddTaskAfterClose{};
    auto_lock_t al(IncomingQueueMutex);
    IncomingQueue.push_back(std::move(task));
  }

  exception_ptr_t& GetException() { return ExceptionPtr; }

  ~TaskHolder() {
    IsClosure = true;
    Thread.join();
  }
};

}  // namespace task
