// task_manager.cpp : Defines the entry point for the application.
//

#include <algorithm>
#include <functional>
#include "task_holder.h"

using namespace std;
using namespace task;

struct SimpleTaskHolderer : public TaskHolder {};

SimpleTaskHolderer Holder;

int main() {
  PostTask(Holder, []() { cout << "Simple task" << endl;
  });

#if 0

  cout << "Test 1 CreateFunctor" << endl;
  {
    int counter = 0;
    // CreateTask using
    CreateFunctor<int, int, int>([&](int i, int j, int k) -> int {
      cout << "call [1/5]" << endl;
      counter++;
      return 0;
    }).Get(1, 2, 3);
    CreateFunctor<int>([&](int i) -> int {
      cout << "call [2/5]" << endl;
      counter++;
      return 0;
    }).Get(1);
    CreateFunctor([&]() -> int {
      cout << "call [3/5]" << endl;
      counter++;
      return 0;
    }).Get();
    CreateFunctor([&]() -> void {
      cout << "call [4/5]" << endl;
      counter++;
    }).Get();
    CreateFunctor([&] {
      cout << "call [5/5]" << endl;
      counter++;
    }).Get();
    if (counter == 5) {
      cout << "ok";
    } else {
      cout << "ERROR!" << endl;
    }
    cout << endl << endl;
  }

  cout << "Test 2 CreateTaskQueue test for case 4" << endl;
  {
    int counter = 0;
    // CreateTask using
    CreateTask([&](int i, int j, int k) -> void {
      cout << "call [1/5]" << endl;
      counter++;
    }, 1, 2, 3).Run();
    CreateTask([&](int i, int j) -> void {
      cout << "call [2/5]" << endl;
      counter++;
    }, 1, 2).Run();
    CreateTask([&](int i) -> void {
      cout << "call [3/5]" << endl;
      counter++;
    }, 1).Run();

    CreateTask([&]() -> void {
      cout << "call [4/5]" << endl;
      counter++;
    }).Run();

    CreateTask([&] {
      cout << "call [5/5]" << endl;
      counter++;
    }).Run();

    if (counter == 5) {
      cout << "ok";
    } else {
      cout << "ERROR!" << endl;
    }
    cout << endl << endl;
  }

  int counter = 0;
  cout << "Test 3 CreateTaskQueue test for case 3, 2, 1" << endl;
  {
    
    // CreateTask 3, 1, 2
    CreateTask([&](int i, int j, int k) -> int {
      cout << "call [1/5]" << endl;
      counter++;
      return 0;
    }, 1, 2, 3).Then([&](int i) -> int {
      cout << "call [2/5]" << endl;
      counter++;
      return 0;
    }).Then([&](int i) -> int {
      cout << "call [3/5]" << endl;
      counter++;
      return 0;
    }).Then([&](int i){
      cout << "call [4/5]" << endl;
      counter++;
    });

  }

  if (counter == 4) {
    cout << "ok";
  } else {
    cout << "ERROR!" << endl;
  }
  cout << endl << endl;

#endif

  /*CreateTask([]() -> void {});
  CreateTask([](int i) -> void {}, 1);
  CreateTask([](int i, int k) -> void {}, 1, 2);

  CreateTask([]() -> int { return 0; });
  CreateTask([](int i) -> int { return 0; }, 1);
  CreateTask([](int i, int k) -> int { return 0; }, 1, 2);*/

  /*auto lambda = [](int i) -> int {
    cout << "Run i = " << i << endl;
    return i + 1;
  };

  cout << "Test 1" << endl;
  Task<decltype(lambda), int, int> task(lambda);
  task.Run(0);
  cout << endl;

  cout << "Test 2 PostTask" << endl;
  {
    PostTask(SimpleHolder, lambda, 13).Then([](int i) {
      cout << "Run2 i = " << i << endl;
      return i + 1;
    });
  }
  cout << endl;

  cout << "Test 3 type test and void argument" << endl;
  {
    struct Foo {};
    struct Bar {};

    PostTask(SimpleHolder,
             [](int i) {
               cout << "Func with void argument posted!" << endl;
               return Bar{};
             },
             13)
        .Then([](Bar i) {
          cout << "child func runned!" << endl;
          return 0;
        });
  }
  cout << endl;

  cout << "Test 4 void return" << endl;
  {
    PostTask(SimpleHolder, [](int i) -> void {
      cout << " void return task runned!" << endl;
      return;
    }, 9);
  }
  cout << endl;

  cout << "Test 5 void void" << endl;
  {
    PostTask(SimpleHolder, []() -> void {
      cout << "void void task runned!" << endl;
      return;
    });
  }
  cout << endl;*/

  getchar();
  return 0;
}
