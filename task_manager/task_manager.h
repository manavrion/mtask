//
// Copyright (C) 2018 Ruslan Manaev (manavrion@yandex.com)
// task_manager version 0.7
// This file is a single-file library and implements simple concurrency
//

#pragma once
#include <atomic>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace manavrion {
namespace task_manager {

using std::move;

template <typename... Ts>
struct Pack {
  using tuple_t = std::tuple<Ts...>;
};

// Deduce Pack from result of function or tuple
// using: typename DeducePack<typename std::result_of<F(As...)>::type>::value
// where: F - function, As... - arguments

template <typename R>
struct DeducePack {
  using value = Pack<R>;
};

template <typename... Rs>
struct DeducePack<std::tuple<Rs...>> {
  using value = Pack<Rs...>;
};

template <>
struct DeducePack<void> {
  using value = Pack<>;
};

// Deduce ResPack from function and arguments
// using: typename DeduceResPack<functor_t, argument_t>::value;
// where: functor_t - function, argument_t - arguments as Pack<As...>

template <typename F, typename A>
struct DeduceResPack {};

template <typename F, typename... As>
struct DeduceResPack<F, Pack<As...>> {
  using value =
      typename DeducePack<typename std::result_of<F(As...)>::type>::value;
};

// Runnable interface for node
template <typename Arg>
struct ITaskNode {
  using argument_tuple_t = Arg;
  virtual void RunNode(argument_tuple_t) = 0;
};

// Runnable interface for task
struct IRunnable {
  virtual void Run() = 0;
};

template <typename F, typename Arg, typename Res>
struct TaskNode;

template <typename F, typename Arg, typename Res>
void FuncRunNode(TaskNode<F, Arg, Res>& node, typename Arg::tuple_t& args) {
  auto res = std::apply(move(node.f), move(args));
  if (node.child_ptr) node.child_ptr->RunNode(move(res));
}

template <typename F, typename Arg>
void FuncRunNode(TaskNode<F, Arg, Pack<>>& node, typename Arg::tuple_t& args) {
  std::apply(move(node.f), move(args));
  if (node.child_ptr) node.child_ptr->RunNode({});
}

template <typename F, typename Arg, typename Res>
struct TaskNode : ITaskNode<typename Arg::tuple_t> {
  using functor_t = F;
  using argument_pack_t = Arg;
  using result_pack_t = Res;

  using argument_tuple_t = typename argument_pack_t::tuple_t;
  using result_tuple_t = typename result_pack_t::tuple_t;

  using child_argument_tuple_t = result_tuple_t;
  using child_node_ptr_t = std::unique_ptr<ITaskNode<child_argument_tuple_t>>;

  functor_t f;
  child_node_ptr_t child_ptr;

  TaskNode(functor_t f) : f(move(f)) {}
  TaskNode(TaskNode&&) = default;

  void RunNode(argument_tuple_t args) final override {
    FuncRunNode(*this, args);
  }
};

template <typename F, typename Arg, typename Res>
struct StarterTaskNode : IRunnable, TaskNode<F, Arg, Res> {
  argument_tuple_t args;

  StarterTaskNode(functor_t f, argument_tuple_t args)
      : TaskNode(move(f)), args(move(args)) {}
  StarterTaskNode(StarterTaskNode&&) = default;

  void Run() final override { RunNode(move(args)); };
};

// Task builders

class TaskHolder;

template <class P>
class TaskBuilder {
  using current_node_t = P;
  using child_node_ptr_t = typename current_node_t::child_argument_tuple_t;

  using current_node_ptr_t = current_node_t*;
  using root_node_ptr_t = std::unique_ptr<IRunnable>;

  TaskHolder& holder;
  root_node_ptr_t root;
  current_node_ptr_t current_ptr;

 public:
  TaskBuilder(TaskHolder& holder, root_node_ptr_t root,
              current_node_ptr_t current_ptr)
      : holder(holder), root(move(root)), current_ptr(current_ptr) {}
  TaskBuilder(TaskBuilder&&) = default;

  template <typename... As, typename... Rs, typename F>
  decltype(auto) ThenImpl(TaskNode<F, Pack<As...>, Pack<Rs...>> node) {
    using child_node_t = TaskNode<F, Pack<As...>, Pack<Rs...>>;
    auto node_rawptr = new child_node_t(move(node));
    current_ptr->child_ptr.reset(node_rawptr);
    return TaskBuilder<child_node_t>(holder, move(root), node_rawptr);
  }

  template <class G>
  decltype(auto) Then(G g) {
    using functor_t = G;
    using argument_t = current_node_t::result_pack_t;
    using result_t = typename DeduceResPack<functor_t, argument_t>::value;
    using node_t = TaskNode<functor_t, argument_t, result_t>;
    return ThenImpl(node_t(move(g)));
  }

  ~TaskBuilder() {
    if (root) holder.AddTask(move(root));
  }
};

template <typename... As, typename... Rs, typename F>
decltype(auto) PostTaskImpl(TaskHolder& holder,
                            StarterTaskNode<F, Pack<As...>, Pack<Rs...>> node) {
  using node_t = StarterTaskNode<F, Pack<As...>, Pack<Rs...>>;
  auto node_rawptr = new node_t(move(node));
  return TaskBuilder<node_t>(holder, std::unique_ptr<node_t>(node_rawptr),
                             node_rawptr);
}

template <class F, class... Args>
decltype(auto) PostTask(TaskHolder& holder, F f, Args... args) {
  using functor_t = F;
  using argument_t = Pack<Args...>;
  using result_t = typename DeduceResPack<functor_t, argument_t>::value;
  using node_t = StarterTaskNode<functor_t, argument_t, result_t>;
  return PostTaskImpl(holder, node_t(move(f), {move(args)...}));
}

// Task holder

struct AddTaskAfterClose : std::exception {};

class TaskHolder {
  using task_ptr_t = std::unique_ptr<IRunnable>;
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

}  // namespace task_manager
}  // namespace manavrion
