// task_manager.cpp : Defines the entry point for the application.
//

#include <algorithm>
#include <functional>
#include "task_holder.h"

using namespace std;
using namespace task;



int main() {

  int counter = 0;

  {
    TaskHolder SimpleTHolder;
    {
      PostTask(SimpleTHolder, [&]() { 
        cout << "TEST 1 : Simple task 1" << endl;
        counter++;
      }).Then([]() {});
      /*.Then([&]() {
        cout << "TEST 1 : Simple task 2" << endl;
        counter++;
      });*/
      
      PostTask(SimpleTHolder, [&]() -> int {
        cout << "TEST 2 : Task stage 1" << endl;
        counter++;
        return 0;
      }).Then([&](int i) { 
        cout << "TEST 2 : Task stage 2" << endl;
        counter++;
      });
      
      PostTask(SimpleTHolder, [&](int i, int j, int k) {
        if (i != 1 || j != 2 || k != 3) {
          cout << "ERROR !\n";
        }
        cout << "TEST 3 : Task stage 1" << endl;
        counter++;
        return 4;
      }, 1, 2, 3).Then([&](int i) {
        if (i != 4) {
          cout << "ERROR !\n";
        }
        cout << "TEST 3 : Task stage 2" << endl;
        counter++;
        return 5;
      }).Then([&](int i) {
        if (i != 5) {
          cout << "ERROR !\n";
        }
        cout << "TEST 3 : Task stage 3" << endl;
        counter++;
      });
    }
  }

  cout << counter << endl;
  if (counter != 7) {
    cout << "FAIL" << endl;
  } else {
    cout << "OK" << endl;
  }

  getchar();
  return 0;
}
