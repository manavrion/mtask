// task_manager.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <iostream>
#include <memory>
#include <tuple>

namespace task {

using std::move;

template <typename... Args>
struct ArgPack {
  using argument_tuple_t = std::tuple<Args...>;
};

template <typename... Res>
struct ResPack {
  using result_tuple_t = std::tuple<Res...>;
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
void FuncRunNode(TaskNode<F, Arg, Res>& node,
                 typename Arg::argument_tuple_t& args) {
  auto res = std::apply(move(node.f), move(args));
  if (node.child_ptr) node.child_ptr->RunNode(move(res));
}

template <typename F, typename Arg>
void FuncRunNode(TaskNode<F, Arg, ResPack<>>& node,
                 typename Arg::argument_tuple_t& args) {
  std::apply(move(node.f), move(args));
  if (node.child_ptr) node.child_ptr->RunNode({});
}

template <typename F, typename Arg, typename Res>
struct TaskNode : ITaskNode<typename Arg::argument_tuple_t> {
  using functor_t = F;
  using argument_tuple_t = typename Arg::argument_tuple_t;
  using result_tuple_t = typename Res::result_tuple_t;

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



}  // namespace task
