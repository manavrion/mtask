// task_manager.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <iostream>
#include <memory>

#include "task_holder.h"
// TODO: Reference additional headers your program requires here.

namespace task {

template <class Arg>
struct ITask {
  virtual void Run(Arg arg) = 0;
};

template <class F, class Arg>
struct Task : public ITask<Arg> {
  F f;

  using argument_t = Arg;
  using functor_t = F;
  using result_t = typename std::result_of<F(Arg)>::type;


  std::unique_ptr<ITask<result_t>> child;

  Task(F f) : f(f) {}
  void Run(Arg arg) override {
    auto res = f(std::move(arg));
    if (child) {
      child->Run(std::move(res));
    }
  }
  Task(Task&&) = default;

  template <class G>
  void Link(G g) {
    child.reset(new Task<G, result_t>(g));
  }
};

template <class F, class Arg>
struct TaskBuilder {
  TaskBuilder(TaskHolder& th, Task<F, Arg> task) : th(th), task(std::move(task)) {}

  TaskBuilder(TaskBuilder&&) = default;

 private:
  Task<F, Arg> task;
  TaskHolder th;

 public:

  using argument_t = Arg;
  using functor_t = F;
  using result_t = typename Task<F, Arg>::result_t;

  template <class G>
  void Then(G g) {
    using arg_local_t = result_t;
    using result_local_t = typename std::result_of<G(arg_local_t)>::type;
    using task_local_t = Task<G, arg_local_t>;
    task.child.reset(new task_local_t(std::move(g)));
  }

  ~TaskBuilder() {
    task.Run(666);
  }
};

template <class F>
decltype(auto) PostTask(TaskHolder& th, F f) {
  using task_t = Task<F, void>;
  return TaskBuilder<F, void>(th, task_t(std::move(f)));
}

template <class F, class Arg>
decltype(auto) PostTask(TaskHolder& th, F f, Arg arg) {
  using task_t = Task<F, Arg>;
  return TaskBuilder<F, Arg>(th, task_t(std::move(f)));
}

}  // namespace task
