// task_manager.cpp : Defines the entry point for the application.
//

#include "task_manager.h"

using namespace std;
using namespace task;

int main() {
  auto lambda = [](int i) -> int {
    cout << "Run i = " << i << endl;
    return i + 1;
  };

  cout << "Test 1" << endl << endl;
  Task<decltype(lambda), int> task(lambda);
  task.Run(0);

  cout << "Test 2" << endl << endl;
  task.Link(lambda);
  task.Run(0);

  cout << "Test 3 PostTask" << endl << endl;
  PostTask(lambda, 13);//.Then([](int i) { return i + 1; })

  getchar();
  return 0;
}
