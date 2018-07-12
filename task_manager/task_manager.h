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
// 1 - start node to end node: contain args, result is void    ||  <class F,
// void, class... Args> 2 - start node to node: contain args, launch other node
// ||  <class F, class Res, class... Args> 4 - end node: result is void ||
// <class F, void, class... Args> 3 - node ||  <class F, class Res, class...
// Args>


// 3 and 4 implement INode
template <class... Args>
struct INode {
  using arguments_t = std::tuple<Args...>;
  virtual void Run(arguments_t) = 0;
};

// 1 and 2 implement ITask
struct ITask {
  virtual void Run() = 0;
};

// nodes

template <class F, class Res, class... Args>
struct TaskNode : INode<Args...> {
  using functor_t = F;
  using result_t = Res;
  using arguments_t = std::tuple<Args...>;

  functor_t f;

  TaskNode(functor_t f) : f(std::move(f)) {}

  void Run(arguments_t args) final override {}
};

template <class F, class... Args>
struct TaskNode<F, void, Args...> : INode<Args...> {
  using functor_t = F;
  using result_t = void;
  using arguments_t = std::tuple<Args...>;

  functor_t f;

  TaskNode(functor_t f) : f(std::move(f)) {}

  void Run(arguments_t args) final override {}
};

// starters

template <class F, class Res, class... Args>
struct Task {
  using functor_t = F;
  using result_t = Res;
  using arguments_t = std::tuple<Args...>;
  using child_t = std::unique_ptr<INode<Args...>>;

  functor_t f;
  
  arguments_t args;

  Task(functor_t f, arguments_t args)
      : f(std::move(f)), args(std::move(args)) {}

  void Run() final override {}
};

template <class F, class... Args>
struct Task<F, void, Args...> : ITask {
  using functor_t = F;
  using result_t = void;
  using arguments_t = std::tuple<Args...>;

  functor_t f;
  arguments_t args;

  Task(functor_t f, arguments_t args)
      : f(std::move(f)), args(std::move(args)) {}

  void Run() final override { std::apply(std::move(f), std::move(args)); }
};

}  // namespace task
