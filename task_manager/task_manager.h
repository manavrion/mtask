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

template <typename... Args>
struct ArgPack<std::tuple<Args...>> {
  using argument_tuple_t = std::tuple<Args...>;
};

template <typename... Res>
struct ResPack {
  using result_tuple_t = std::tuple<Res...>;
};

template <typename Res>
struct DeduceResPack {
  using value = ResPack<Res>;
};

template <>
struct DeduceResPack<void> {
  using value = ResPack<>;
};

template <typename F, typename Args>
struct DeduceResPack2 {
  using value = ResPack<>;
};

template <typename F, typename ... Args>
struct DeduceResPack2<F, std::tuple<Args ...>> {
  using value =
      typename DeduceResPack<typename std::result_of<F(Args...)>::type>::value;
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

// Task builders

class TaskHolder;

// node
template <class P, class C>
class TaskBuilder {
  using current_node_t = P;
  using child_node_ptr_t = C;

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

  template <class G>
  decltype(auto) Then(G g) {

    using functor_t = G;
    using argument_tuple_t = ArgPack<current_node_t::child_argument_tuple_t>;
    using result_tuple_t = typename DeduceResPack2<G, argument_tuple_t>::value;

    using node_t = TaskNode<functor_t, argument_tuple_t, result_tuple_t>;
    auto node_ptr = new node_t(move(g));

    current_ptr->child_ptr.reset(node_ptr);

    return TaskBuilder<node_t, node_t::child_argument_tuple_t>(
        holder, move(root), node_ptr);
  }

  ~TaskBuilder() {
    if (root) holder.AddTask(move(root));
  }
};

template <class F, class... Args>
decltype(auto) PostTask(TaskHolder& th, F f, Args... args) {

  using functor_t = F;
  using argument_tuple_t = ArgPack<Args...>;
  using result_tuple_t = typename DeduceResPack<typename std::result_of<F(Args...)>::type>::value;

  using node_t = StarterTaskNode<functor_t, argument_tuple_t, result_tuple_t>;
  auto node_ptr = new node_t(move(f), {move(args)...});

  return TaskBuilder<node_t, node_t::child_argument_tuple_t>(
      th, std::unique_ptr<node_t>(node_ptr), node_ptr);
}

}  // namespace task
