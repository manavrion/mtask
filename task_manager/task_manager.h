// task_manager.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <iostream>
#include <memory>
#include <tuple>

namespace task {

using std::move;

// type of tasks node:
// 1 - end node: result is void
// 2 - node
// 3 - start node to end node: contain args, result is void
// 4 - start node to node: contain args, launch other node

// Runnable interface for node
template <class... Args>
struct INode {
  virtual void Run(Args...) = 0;
};

// Runnable interface for task
struct ITask {
  virtual void Run() = 0;
};

// node

template <class F, class Res, class... Args>
struct TaskNode : INode<Args...> {
  using functor_t = F;
  using result_t = Res;
  using argument_tuple_t = std::tuple<Args...>;

  using child_argument_t = result_t;
  using child_node_ptr_t = std::unique_ptr<INode<child_argument_t>>;

  functor_t f;
  child_node_ptr_t child_ptr;

  TaskNode(functor_t f) : f(move(f)) {}
  TaskNode(TaskNode&&) = default;

  void Run(Args... args) final override {
    auto res = f(move(args)...);
    if (child_ptr) child_ptr->Run(move(res));
  }
};

// end node
template <class F, class... Args>
struct TaskNode<F, void, Args...> : INode<Args...> {
  using Res = void;

  using functor_t = F;
  using result_t = Res;
  using argument_tuple_t = std::tuple<Args...>;

  using child_argument_t = result_t;
  using child_node_ptr_t = std::unique_ptr<INode<child_argument_t>>;

  functor_t f;
  child_node_ptr_t child_ptr;

  TaskNode(functor_t f) : f(move(f)) {}
  TaskNode(TaskNode&&) = default;

  void Run(Args... args) final override {
    f(move(args)...);
    if (child_ptr) child_ptr->Run();
  }
};

// starters

// starter node
template <class F, class Res, class... Args>
struct Task : ITask {
  using functor_t = F;
  using result_t = Res;
  using argument_tuple_t = std::tuple<Args...>;

  using child_argument_t = result_t;
  using child_node_ptr_t = std::unique_ptr<INode<child_argument_t>>;

  functor_t f;
  argument_tuple_t args;
  child_node_ptr_t child_ptr;

  Task(functor_t f, Args... args) : f(move(f)), args(move(args)...) {}
  Task(Task&&) = default;

  void Run() final override {
    auto res = std::apply(move(f), move(args));
    if (child_ptr) child_ptr->Run(move(res));
  }
};

// simple starter
template <class F, class... Args>
struct Task<F, void, Args...> : ITask {
  using Res = void;
  using functor_t = F;
  using result_t = Res;
  using argument_tuple_t = std::tuple<Args...>;

  using child_argument_t = result_t;
  using child_node_ptr_t = std::unique_ptr<INode<child_argument_t>>;

  functor_t f;
  argument_tuple_t args;
  child_node_ptr_t child_ptr;

  Task(functor_t f, Args... args) : f(move(f)), args(move(args)...) {}
  Task(Task&&) = default;

  void Run() final override {
    std::apply(move(f), move(args));
    if (child_ptr) child_ptr->Run();
  }
};

// builders

class TaskHolder;

// node
template <class P, class C>
class TaskBuilder {
  using current_node_t = P;
  using child_node_ptr_t = C;

  using current_node_ptr_t = current_node_t*;
  using root_node_ptr_t = std::unique_ptr<ITask>;

  TaskHolder& th;
  root_node_ptr_t root;
  current_node_ptr_t current_ptr;

 public:
  TaskBuilder(TaskHolder& th, root_node_ptr_t root,
              current_node_ptr_t current_ptr)
      : th(th), root(move(root)), current_ptr(current_ptr) {}
  TaskBuilder(TaskBuilder&&) = default;

  template <class G>
  decltype(auto) Then(G g) {
    using functor_lt = G;
    using argument_lt = current_node_t::child_argument_t;
    using result_lt = typename std::result_of<functor_lt(argument_lt)>::type;

    using tasknode_lt = TaskNode<functor_lt, result_lt, argument_lt>;
    auto tasknode_ptr = new tasknode_lt(move(g));
    current_ptr->child_ptr.reset(tasknode_ptr);

    return TaskBuilder<tasknode_lt, tasknode_lt::child_node_ptr_t>(
        th, move(root), tasknode_ptr);
  }

  ~TaskBuilder() {
    if (root) th.AddTask(move(root));
  }
};

// end
template <class P>
class TaskBuilder<P, void> {
  using current_node_t = P;
  using child_node_ptr_t = void;

  using current_node_ptr_t = current_node_t*;
  using root_node_ptr_t = std::unique_ptr<ITask>;

  TaskHolder& th;
  root_node_ptr_t root;
  current_node_ptr_t current_ptr;

 public:
  TaskBuilder(TaskHolder& th, root_node_ptr_t root,
              current_node_ptr_t current_ptr)
      : th(th), root(move(root)), current_ptr(current_ptr) {}
  TaskBuilder(TaskBuilder&&) = default;

    template <class G>
  decltype(auto) Then(G g) {
    using functor_t = G;
    using argument_t = current_node_t::child_argument_t;
    using result_t = typename std::result_of<functor_t()>::type;

    using node_t = TaskNode<functor_t, result_t>;
    //auto node_ptr = new node_t(move(g));
    //current_ptr->child_ptr.reset(node_ptr);
    /*
    return TaskBuilder<tasknode_lt, tasknode_lt::child_node_ptr_t>(
        th, move(root), tasknode_ptr);*/
  }

  ~TaskBuilder() {
    if (root) th.AddTask(move(root));
  }
};

template <class F, class... Args>
decltype(auto) PostTask(TaskHolder& th, F f, Args... args) {
  using result_t = typename std::result_of<F(Args...)>::type;
  using task_t = Task<F, result_t, Args...>;
  auto task_ptr = new task_t(move(f), move(args)...);
  return TaskBuilder<task_t, task_t::child_argument_t>(
      th, std::unique_ptr<task_t>(task_ptr), task_ptr);
}

}  // namespace task
