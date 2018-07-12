// task_manager.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <iostream>
#include <memory>
#include <tuple>

// TODO: Reference additional headers your program requires here.

namespace task {

// Tasks wrappers, contains also queued task

template <class Res, class... Args>
struct IFunctor {
  using result_t = Res;
  using arguments_t = std::tuple<Args...>;

  virtual result_t Get(Args... args) = 0;
  virtual result_t Get(arguments_t args) = 0;
};

template <class F, class Res, class... Args>
struct Functor {
  using functor_t = F;
  using result_t = Res;
  using arguments_t = std::tuple<Args...>;

  functor_t f;

  Functor(F f) : f(std::move(f)) {}
  Functor(Functor&&) = default;

  result_t Get(Args... args) { return f(std::move(args)...); }
  result_t Get(arguments_t args) {
    return std::apply(std::move(f), std::move(args));
  }
};

template <class... Args, class F>
decltype(auto) CreateFunctor(F f) {
  using result_t = typename std::result_of<F(Args...)>::type;
  using task_t = Functor<F, result_t, Args...>;
  return task_t(std::move(f));
}

// type of tasks node:
// 1 - end node: result is void
// 2 - node
// 3 - start node to end node: contain args, result is void
// 4 - start node to node: contain args, launch other node

// 1 and 2 implement INode
template <class... Args>
struct INode {
  virtual void Run(Args...) = 0;
};

// 3 and 4 implement ITask
struct ITask {
  virtual void Run() = 0;
};

// node

template <class F, class Res, class... Args>
struct TaskNode : INode<Args...> {
  using functor_t = F;
  using result_t = Res;
  using child_arg_t = result_t;
  using child_t = std::unique_ptr<INode<child_arg_t>>;

  functor_t f;
  child_t child;

  TaskNode(functor_t f) : f(std::move(f)) {}

  void Run(Args... args) final override {
    auto res = f(std::move(args)...);
    if (child) child->Run(std::move(res));
  }

  // deprecated
  /*template <class G>
  decltype(auto) Then(G g) {
    using functor_lt = G;
    using argument_lt = result_t;
    using result_lt = typename std::result_of<functor_lt(argument_lt)>::type;
    using tasknode_lt = TaskNode<functor_lt, result_lt, argument_lt>;
    auto tasknode_ptr = new tasknode_lt(std::move(g));
    child.reset(tasknode_ptr);
    return *tasknode_ptr;
  }*/
};

// end node
template <class F, class... Args>
struct TaskNode<F, void, Args...> : INode<Args...> {
  using functor_t = F;
  using result_t = void;
  using child_arg_t = void;

  functor_t f;

  TaskNode(functor_t f) : f(std::move(f)) {}

  void Run(Args... args) final override { f(std::move(args)...); }
};

// starters

// starter node
template <class F, class Res, class... Args>
struct Task : ITask {
  using functor_t = F;
  using result_t = Res;
  using arguments_t = std::tuple<Args...>;
  using child_arg_t = result_t;
  using child_t = std::unique_ptr<INode<child_arg_t>>;

  functor_t f;
  arguments_t args;
  child_t child;

  Task(functor_t f, Args... args) : f(std::move(f)), args(std::move(args)...) {}
  Task(Task&&) = default;

  void Run() final override {
    auto res = std::apply(std::move(f), std::move(args));
    if (child) child->Run(std::move(res));
  }

  //deprecated
  /*template <class G>
  decltype(auto) Then(G g) {
    using functor_lt = G;
    using argument_lt = result_t;
    using result_lt = typename std::result_of<functor_lt(argument_lt)>::type;
    using tasknode_lt = TaskNode<functor_lt, result_lt, argument_lt>;
    auto tasknode_ptr = new tasknode_lt(std::move(g));
    child.reset(tasknode_ptr);
    return *tasknode_ptr;
  }*/
};

// simple starter
template <class F, class... Args>
struct Task<F, void, Args...> : ITask {
  using functor_t = F;
  using result_t = void;
  using arguments_t = std::tuple<Args...>;
  using child_arg_t = void;

  functor_t f;
  arguments_t args;

  Task(functor_t f, Args... args) : f(std::move(f)), args(std::move(args)...) {}
  Task(Task&&) = default;

  void Run() final override { std::apply(std::move(f), std::move(args)); }
};

template <class F, class... Args>
decltype(auto) CreateTask(F f, Args... args) {
  using result_t = typename std::result_of<F(Args...)>::type;
  using task_t = Task<F, result_t, Args...>;
  return task_t(std::move(f), std::move(args)...);
}

// builders

class TaskHolder;

// node
template <class task_current_t, class child_arg_t>
class TaskBuilder {
  using root_task_ptr_t = std::unique_ptr<ITask>;
  using task_child_t = std::unique_ptr<INode<child_arg_t>>;

  TaskHolder& th;
  root_task_ptr_t root;

  task_current_t* current;
  task_child_t child;

 public:
  TaskBuilder(TaskHolder& th, root_task_ptr_t root, task_current_t* current)
      : th(th), root(std::move(root)), current(current) {}
  TaskBuilder(TaskBuilder&&) = default;

  template <class G>
  decltype(auto) Then(G g) {
    using functor_lt = G;
    using argument_lt = child_arg_t;
    using result_lt = typename std::result_of<functor_lt(argument_lt)>::type;

    using tasknode_lt = TaskNode<functor_lt, result_lt, argument_lt>;
    auto tasknode_ptr = new tasknode_lt(std::move(g));
    current->child.reset(tasknode_ptr);

    return TaskBuilder<tasknode_lt, tasknode_lt::child_arg_t>(
        th, std::move(root), tasknode_ptr);
  }

  ~TaskBuilder() {
    if (root) th.AddTask(std::move(root));
  }
};

// end
template <class task_current_t>
class TaskBuilder<task_current_t, void> {
  using root_task_ptr_t = std::unique_ptr<ITask>;
  using task_child_t = void;

  TaskHolder& th;
  root_task_ptr_t root;

  task_current_t* current;

 public:
  TaskBuilder(TaskHolder& th, root_task_ptr_t root, task_current_t* current)
      : th(th), root(std::move(root)), current(current) {}
  TaskBuilder(TaskBuilder&&) = default;

  ~TaskBuilder() { th.AddTask(std::move(root)); }
};

template <class F, class... Args>
decltype(auto) PostTask(TaskHolder& th, F f, Args... args) {
  using result_t = typename std::result_of<F(Args...)>::type;
  using task_t = Task<F, result_t, Args...>;
  auto task_ptr = new task_t(std::move(f), std::move(args)...);
  return TaskBuilder<task_t, task_t::child_arg_t>(
      th, std::unique_ptr<task_t>(task_ptr), task_ptr);
}

}  // namespace task
