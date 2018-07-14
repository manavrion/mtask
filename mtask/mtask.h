//
// Copyright (C) 2018 Ruslan Manaev (manavrion@yandex.com)
// mtask version 0.8
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
namespace mtask {

using std::move;

template <typename... Ts>
struct Pack {
  using tuple_t = std::tuple<Ts...>;
};

// Deduce Pack from result of function or tuple
// using: 
// typename DeducePack<decltype(std::declval<F>()(std::declval<As>()...))>::type 
// where: F - function, As... - arguments

template <typename R>
struct DeducePack {
  using type = Pack<R>;
};

template <typename... Rs>
struct DeducePack<std::tuple<Rs...>> {
  using type = Pack<Rs...>;
};

template <>
struct DeducePack<void> {
  using type = Pack<>;
};

// Deduce ResPack from function and arguments
// using: typename DeduceResPack<functor_t, argument_t>::type;
// where: functor_t - function, argument_t - arguments as Pack<As...>

template <typename F, typename A>
struct DeduceResPack;

template <typename F, typename... As>
struct DeduceResPack<F, Pack<As...>> {
  using type = typename DeducePack<decltype(
      std::declval<F>()(std::declval<As>()...))>::type;
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
  auto ThenImpl(TaskNode<F, Pack<As...>, Pack<Rs...>> node) {
    using child_node_t = TaskNode<F, Pack<As...>, Pack<Rs...>>;
    auto node_rawptr = new child_node_t(move(node));
    current_ptr->child_ptr.reset(node_rawptr);
    return TaskBuilder<child_node_t>(holder, move(root), node_rawptr);
  }

  template <class G>
  auto Then(G g) {
    using functor_t = G;
    using argument_t = current_node_t::result_pack_t;
    using result_t = typename DeduceResPack<functor_t, argument_t>::type;
    using node_t = TaskNode<functor_t, argument_t, result_t>;
    return ThenImpl(node_t(move(g)));
  }

  ~TaskBuilder() {
    if (root) holder.AddTask(move(root));
  }
};

template <typename... As, typename... Rs, typename F>
auto PostTaskImpl(TaskHolder& holder,
                            StarterTaskNode<F, Pack<As...>, Pack<Rs...>> node) {
  using node_t = StarterTaskNode<F, Pack<As...>, Pack<Rs...>>;
  auto node_rawptr = new node_t(move(node));
  return TaskBuilder<node_t>(holder, std::unique_ptr<node_t>(node_rawptr),
                             node_rawptr);
}

template <class F, class... Args>
auto PostTask(TaskHolder& holder, F f, Args... args) {
  using functor_t = F;
  using argument_t = Pack<Args...>;
  using result_t = typename DeduceResPack<functor_t, argument_t>::type;
  using node_t = StarterTaskNode<functor_t, argument_t, result_t>;
  return PostTaskImpl(holder, node_t(move(f), {move(args)...}));
}

// Task holder

struct AddTaskAfterClose : std::exception {};

class TaskHolder {
  using task_ptr_t = std::unique_ptr<IRunnable>;
  using task_queue_t = std::vector<task_ptr_t>;
  using auto_lock_t = std::lock_guard<std::mutex>;

  task_queue_t IncomingQueue;
  std::mutex IncomingQueueMutex;

  std::atomic<bool> IsDestructing;
  std::exception_ptr ExceptionPtr;
  std::thread Thread;

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
          if (IsDestructing) break;
          sleep = true;
        }
      }
      // bad code
      if (sleep) std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
  }

 public:
  TaskHolder()
      : IsDestructing(false), Thread(std::bind(&TaskHolder::Loop, this)) {}

  void AddTask(task_ptr_t task) {
    if (IsDestructing) throw AddTaskAfterClose{};
    auto_lock_t al(IncomingQueueMutex);
    IncomingQueue.push_back(std::move(task));
  }

  std::exception_ptr& GetException() { return ExceptionPtr; }

  ~TaskHolder() {
    IsDestructing = true;
    Thread.join();
  }
};

}  // namespace mtask
}  // namespace manavrion
