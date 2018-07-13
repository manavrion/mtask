// task_manager.cpp : Defines the entry point for the application.
//

#include <algorithm>
#include <functional>
#include "task_holder.h"

using namespace std;
using namespace task;

int main() {
  // only TaskNode tests
  {
    auto f1 = [](int i, int j) {
      cout << "pre test 1/4" << endl;
      return make_tuple(i + 1, j + 1);
    };
    TaskNode<decltype(f1), ArgPack<int, int>, ResPack<int, int>> tn1(move(f1));
    tn1.RunNode({1, 2});

    auto f2 = []() { cout << "pre test 2/4" << endl; };
    TaskNode<decltype(f2), ArgPack<>, ResPack<>> tn2(move(f2));
    tn2.RunNode({});

    auto f3 = [](int i) { cout << "pre test 3/4" << endl; };
    TaskNode<decltype(f3), ArgPack<int>, ResPack<>> tn3(move(f3));
    tn3.RunNode({1});

    auto f4 = []() {
      cout << "pre test 4/4" << endl;
      return make_tuple(1);
    };
    TaskNode<decltype(f4), ArgPack<>, ResPack<int>> tn4(move(f4));
    tn4.RunNode({});
  }

  {
    auto f1 = []() {
      cout << "pre II test 1/4" << endl;
      return make_tuple(1, 2);
    };
    auto f2 = [](int i, int j) { cout << "pre II test 2/4" << endl; };
    TaskNode<decltype(f1), ArgPack<>, ResPack<int, int>> tn1(move(f1));
    tn1.child_ptr.reset(
        new TaskNode<decltype(f2), ArgPack<int, int>, ResPack<>>(move(f2)));
    tn1.RunNode({});
  }

  {
    struct Bar {};

    auto f1 = []() -> Bar {
      cout << "pre II test 3/4" << endl;
      return Bar{};
    };
    auto f2 = [](Bar i) { cout << "pre II test 4/4" << endl; };
    TaskNode<decltype(f1), ArgPack<>, ResPack<Bar>> tn1(move(f1));
    tn1.child_ptr.reset(
        new TaskNode<decltype(f2), ArgPack<Bar>, ResPack<>>(move(f2)));
    tn1.RunNode({});
  }

  // StarterTaskNode tests
  {
    auto f1 = [](int i, int j) {
      cout << "pre III test 1/8" << endl;
      return make_tuple(i + 1, j + 1);
    };
    StarterTaskNode<decltype(f1), ArgPack<int, int>, ResPack<int, int>> tn1(
        move(f1), {1, 2});
    tn1.Run();

    auto f2 = []() { cout << "pre III test 2/8" << endl; };
    StarterTaskNode<decltype(f2), ArgPack<>, ResPack<>> tn2(move(f2), {});
    tn2.Run();

    auto f3 = [](int i) { cout << "pre III test 3/8" << endl; };
    StarterTaskNode<decltype(f3), ArgPack<int>, ResPack<>> tn3(move(f3), {1});
    tn3.Run();

    auto f4 = []() {
      cout << "pre III test 4/8" << endl;
      return make_tuple(1);
    };
    StarterTaskNode<decltype(f4), ArgPack<>, ResPack<int>> tn4(move(f4), {});
    tn4.Run();
  }

  {
    auto f1 = []() {
      cout << "pre III test 5/8" << endl;
      return make_tuple(1, 2);
    };
    auto f2 = [](int i, int j) { cout << "pre III test 6/8" << endl; };
    StarterTaskNode<decltype(f1), ArgPack<>, ResPack<int, int>> tn1(move(f1),
                                                                    {});
    tn1.child_ptr.reset(
        new TaskNode<decltype(f2), ArgPack<int, int>, ResPack<>>(move(f2)));
    tn1.Run();
  }

  {
    struct Bar {};

    auto f1 = []() -> Bar {
      cout << "pre III test 7/8" << endl;
      return Bar{};
    };
    auto f2 = [](Bar i) { cout << "pre III test 8/8" << endl; };
    StarterTaskNode<decltype(f1), ArgPack<>, ResPack<Bar>> tn1(move(f1), {});
    tn1.child_ptr.reset(
        new TaskNode<decltype(f2), ArgPack<Bar>, ResPack<>>(move(f2)));
    tn1.Run();
  }

  int counter = 0;

  {
    TaskHolder SimpleTHolder;
    {
      /*PostTask(SimpleTHolder, [&]() {
        cout << "TEST 1 : Simple task 1" << endl;
        counter++;
      }).Then([]() {});*/
      /*.Then([&]() {
        cout << "TEST 1 : Simple task 2" << endl;
        counter++;
      });*/

      /*PostTask(SimpleTHolder, [&]() -> int {
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
      });*/
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
