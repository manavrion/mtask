// task_manager.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <iostream>
#include <memory>
#include <tuple>
#include <string>
#include <typeinfo>  //tmp

namespace task {

using std::move;

// Pack of arguments of function

template <typename... As>
struct ArgPack {
  using argument_tuple_t = std::tuple<As...>;
};

template <typename... As>
struct ArgPack<std::tuple<As...>> {
  using argument_tuple_t = std::tuple<As...>;
};

//

// Pack of results of function

template <typename... Rs>
struct ResPack {
  using result_tuple_t = std::tuple<Rs...>;
};

//

// Cast ResPack to ArgPack

template <typename A>
struct ArgCast {};

template <typename... Rs>
struct ArgCast<ResPack<Rs...>> {
  using value = ArgPack<Rs...>;
};

//

// Deduce ResPack from result of function
// using: DeduceResPack<typename std::result_of<F(As...)>::type>
// where: F - function, Args.. - arguments

template <typename R>
struct DeduceResPack {
  using value = ResPack<R>;
};

template <typename ... Rs>
struct DeduceResPack<std::tuple<Rs...>> {
  using value = ResPack<Rs...>;
};

template <>
struct DeduceResPack<void> {
  using value = ResPack<>;
};

//

template <typename F, typename A>
struct DeduceResPackFromArgPack {};

template <typename F, typename ... As>
struct DeduceResPackFromArgPack<F, ArgPack<As...>> {
  using value = typename DeduceResPack<typename std::result_of<F(As...)>::type>::value;
};

template <typename F, typename... As>
struct DeduceResPackFromArgPack<F, ArgPack<std::tuple<As...>>> {
  using value = typename DeduceResPack<typename std::result_of<F(As...)>::type>::value;
};

//


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
  using result_pack_t = Res;

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
  decltype(auto) ThenImpl(TaskNode<F, ArgPack<As...>, ResPack<Rs...>> node) {
    using child_node_t = TaskNode<F, ArgPack<As...>, ResPack<Rs...>>;
    auto node_rawptr = new child_node_t(move(node));
    current_ptr->child_ptr.reset(node_rawptr);
    return TaskBuilder<child_node_t>(holder, move(root), node_rawptr);
  }

  template <class G>
  decltype(auto) Then(G g) {
    using functor_t = G;
    using argument_t = ArgCast<current_node_t::result_pack_t>::value;
    using result_t =
        typename DeduceResPackFromArgPack<functor_t, argument_t>::value;
    using node_t = TaskNode<functor_t, argument_t, result_t>;
    return ThenImpl(node_t(move(g)));
  }

  ~TaskBuilder() {
    if (root) holder.AddTask(move(root));
  }
};

template <typename... As, typename... Rs, typename F>
decltype(auto) PostTaskImpl(
    TaskHolder& holder,
    StarterTaskNode<F, ArgPack<As...>, ResPack<Rs...>> node) {
  using node_t = StarterTaskNode<F, ArgPack<As...>, ResPack<Rs...>>;
  auto node_rawptr = new node_t(move(node));
  return TaskBuilder<node_t>(holder, std::unique_ptr<node_t>(node_rawptr),
                             node_rawptr);
}

template <class F, class... Args>
decltype(auto) PostTask(TaskHolder& holder, F f, Args... args) {
  using functor_t = F;
  using argument_t = ArgPack<Args...>;
  using result_t =
      typename DeduceResPackFromArgPack<functor_t, argument_t>::value;
  using node_t = StarterTaskNode<functor_t, argument_t, result_t>;
  return PostTaskImpl(holder, node_t(move(f), {move(args)...}));
}

}  // namespace task
