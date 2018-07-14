// task_manager.cpp : Defines the entry point for the application.
//

#include <algorithm>
#include <functional>
#include <typeinfo>
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
    auto f2 = [](int i, int j) {
      cout << "pre II test 2/4" << endl;
      return make_tuple(3, 4);
    };
    auto f3 = [](int i, int j) { cout << "pre II test 2/4" << endl; };

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
    auto f2 = [](int i, int j) {
      cout << "pre III test 6/8   1" << endl;
      return make_tuple(1, 2);
    };
    auto f3 = [](int i, int j) {
      cout << "pre III test 6/8   2" << endl;
      return make_tuple(1, 2);
    };
    auto f4 = [](int i, int j) { cout << "pre III test 6/8   3" << endl; };

    StarterTaskNode<decltype(f1), ArgPack<>, ResPack<int, int>> tn1(move(f1),
                                                                    {});

    auto p2 = new TaskNode<decltype(f2), ArgPack<int, int>, ResPack<int, int>>(
        move(f2));
    auto p3 = new TaskNode<decltype(f3), ArgPack<int, int>, ResPack<int, int>>(
        move(f3));
    auto p4 =
        new TaskNode<decltype(f4), ArgPack<int, int>, ResPack<>>(move(f4));

    p2->child_ptr.reset(p3);
    p3->child_ptr.reset(p4);

    tn1.child_ptr.reset(p2);

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
      auto f1_1 = [&]() {
        cout << "TEST 1 : Simple task 1" << endl;
        counter++;
      };
      auto f1_2 = [&]() {
        cout << "TEST 1 : Simple task 2" << endl;
        counter++;
      };
      auto f1_3 = [&]() {
        cout << "TEST 1 : Simple task 3" << endl;
        counter++;
      };

      StarterTaskNode<decltype(f1_1), ArgPack<>, ResPack<>> g1_1(move(f1_1), {});
      TaskNode<decltype(f1_2), ArgPack<>, ResPack<>> g1_2(move(f1_2));
      TaskNode<decltype(f1_3), ArgPack<>, ResPack<>> g1_3(move(f1_3));

      PostTaskImpl(SimpleTHolder, move(g1_1))
          .ThenImpl(move(g1_2))
          .ThenImpl(move(g1_3));

      PostTask(SimpleTHolder, f1_1).Then(f1_2).Then(f1_3);

      //////////

      auto f2_1 = [&](int i) -> int {
        cout << "TEST 2 : Simple task 1" << endl;
        counter++;
        return 1;
      };
      auto f2_2 = [&](int i) -> int {
        cout << "TEST 2 : Simple task 2" << endl;
        counter++;
        return 2;
      };
      auto f2_3 = [&](int i) -> int {
        cout << "TEST 2 : Simple task 3" << endl;
        counter++;
        return 3;
      };
      auto f2_4 = [&](int i) -> void {
        cout << "TEST 2 : Simple task 4" << endl;
        counter++;
      };

      StarterTaskNode<decltype(f2_1), ArgPack<int>, ResPack<int>> g2_1(move(f2_1), {0});
      TaskNode<decltype(f2_2), ArgPack<int>, ResPack<int>> g2_2(move(f2_2));
      TaskNode<decltype(f2_3), ArgPack<int>, ResPack<int>> g2_3(move(f2_3));
      TaskNode<decltype(f2_4), ArgPack<int>, ResPack<>> g2_4(move(f2_4));

      PostTaskImpl(SimpleTHolder, move(g2_1))
          .ThenImpl(move(g2_2))
          .ThenImpl(move(g2_3))
          .ThenImpl(move(g2_4));

      PostTask(SimpleTHolder, f2_1, 1).Then(f2_2).Then(f2_3).Then(f2_4);

      ////

      auto f3_1 = [&](int i, int j, int k) {
        cout << "TEST 3 : Simple task 1" << endl;
        counter++;
        return make_tuple(1, 2, 3);
      };
      auto f3_2 = [&](int i, int j, int k) {
        cout << "TEST 3 : Simple task 2" << endl;
        counter++;
        return make_tuple(1, 2, 3);
      };
      auto f3_3 = [&](int i, int j, int k) {
        cout << "TEST 3 : Simple task 3" << endl;
        counter++;
        return make_tuple(1, 2, 3);
      };
      auto f3_4 = [&](int i, int j, int k) -> void {
        cout << "TEST 3 : Simple task 4" << endl;
        counter++;
      };

      StarterTaskNode<decltype(f3_1), ArgPack<int, int, int>, ResPack<int, int, int>> g3_1(f3_1, {1, 2, 3});
      TaskNode<decltype(f3_2), ArgPack<int, int, int>, ResPack<int, int, int>> g3_2(f3_2);
      TaskNode<decltype(f3_3), ArgPack<int, int, int>, ResPack<int, int, int>> g3_3(f3_3);
      TaskNode<decltype(f3_4), ArgPack<int, int, int>, ResPack<>> g3_4(f3_4);

      PostTaskImpl(SimpleTHolder, move(g3_1))
          .ThenImpl(move(g3_2))
          .ThenImpl(move(g3_3))
          .ThenImpl(move(g3_4));

      PostTask(SimpleTHolder, f3_1, 1, 2, 3).Then(f3_2).Then(f3_3).Then(f3_4);

    }
  }

  if (counter != 11*2) {
    cout << "T FAIL" << endl;
  } else {
    cout << "T OK" << endl;
  }

  cout << endl;
  cout << endl;

  counter = 0;

  {
    TaskHolder SimpleTHolder;
    {
      PostTask(SimpleTHolder, [&]() {
        cout << "TEST 1 : Simple task 1" << endl;
        counter++;
      }).Then([&]() {
        cout << "TEST 1 : Simple task 2" << endl;
        counter++;
      }).Then([&]() {
        cout << "TEST 1 : Simple task 3" << endl;
        counter++;
      });

      PostTask(SimpleTHolder, [&]() -> int {
        cout << "TEST 2 : Task stage 1" << endl;
        counter++;
        return 0;
      }).Then([&](int i) {
        cout << "TEST 2 : Task stage 2" << endl;
        counter++;
        return 1;
      }).Then([&](int i) {
        cout << "TEST 2 : Task stage 3" << endl;
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

      PostTask(SimpleTHolder, [&](int i, int j, int k) {
        if (i != 1 || j != 2 || k != 3) cout << "ERROR !\n";        
        cout << "TEST 4 : Task stage 1" << endl;
        counter++;
        return make_tuple(i + 1, j + 1, k + 1);
      }, 1, 2, 3).Then([&](int i, int j, int k) {
        if (i != 2 || j != 3 || k != 4) cout << "ERROR !\n";       
        cout << "TEST 4 : Task stage 2" << endl;
        counter++;
        return make_pair(1, 2);
      }).Then([&](std::pair<int, int> pair) {
        cout << "TEST 4 : Task stage 3" << endl;
        counter++;
        return make_tuple(1);
      }).Then([&](int i) {
        cout << "TEST 4 : Task stage 4" << endl;
        counter++;
        return 1;
      }).Then([&](int i) {
        cout << "TEST 4 : Task stage 5" << endl;
        counter++;
        return 1;
      });
    }
  }

  cout << counter << endl;
  if (counter != 14) {
    cout << "FAIL" << endl;
  } else {
    cout << "OK" << endl;
  }

  getchar();
  return 0;
}
