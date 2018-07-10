// task_manager.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <iostream>
#include <memory>

// TODO: Reference additional headers your program requires here.

namespace task {

template <class Arg>
struct ITask {
  virtual void Run(Arg arg) = 0;
};

template <class F, class Arg>
struct Task : public ITask<Arg> {
 private:
  F f;

 public:
  using argument_t = Arg;
  using functor_t = F;
  using result_t = typename std::result_of<F(Arg)>::type;

 private:
  std::unique_ptr<ITask<result_t>> child;

 public:
  Task(F f) : f(f) {}
  void Run(Arg arg) override {
    auto res = f(std::move(arg));
    if (child) {
      child->Run(std::move(res));
    }
  }

  template <class G>
  void Link(G g) {
    child.reset(new Task<G, result_t>(g));
  }
};

template <class F, class Arg>
struct TaskBuilder {
  TaskBuilder(Task<F, Arg> task) : task(std::move(task)) {}
  Task<F, Arg> task;

  //template <class G>
  //TaskBuilder<Task<G, T::result_t>> Then(G g) {}

  ~TaskBuilder() {
    task.Run(666);
  }
};

template <class F, class Arg>
decltype(auto) PostTask(F f, Arg arg) {
  using result = decltype(f(arg));
  using task_t = Task<F, Arg>;
  return std::move(TaskBuilder<F, Arg>(task_t(std::move(f))));
}

}  // namespace task
