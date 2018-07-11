// task_manager.cpp : Defines the entry point for the application.
//

#include "task_manager.h"

using namespace std;
using namespace task;

struct SimpleTaskHolderer : public TaskHolder {};

SimpleTaskHolderer SimpleHolder;

int main() {
  auto lambda = [](int i) -> int {
    cout << "Run i = " << i << endl;
    return i + 1;
  };

  /*cout << "Test 1" << endl << endl;
  Task<decltype(lambda), int> task(lambda);
  task.Run(0);

  cout << "Test 2" << endl << endl;
  task.Link(lambda);
  task.Run(0);*/

  cout << "Test 3 PostTask" << endl << endl;
  {
    PostTask(SimpleHolder, lambda, 13).Then([](int i) {
      cout << "Run2 i = " << i << endl;
      return i + 1;
    });
  }

  cout << "Test 4 PostTask" << endl << endl;
  {
    struct Foo {};
    struct Bar {};

    PostTask(SimpleHolder, [](int i) { return Bar{}; }, 13).Then([](Bar i) {
      
      return Foo{};
    });
  }

  cout << "Test 5 PostTask" << endl << endl;
  {
    // PostTask(SimpleHolder, []() -> int { return 1; });
  }

  getchar();
  return 0;
}
