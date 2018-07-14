# mtask
mtask is a single-file library and implements simple concurrency

## Example
### Define task holder

```CPP
TaskHolder holder;
```
Also you can define your own holder, just extends TaskHolder

### Posting tasks

#### simple task
```CPP
PostTask(holder, []() {
  // code
});
```

#### task with arguments
```CPP
PostTask(holder, [](int i, int j) {
  // code
}, 0, 1);
```

#### tasks queue
```CPP
PostTask(holder, []() {
  // code
}).Then([](){
  // code
  return Foo{};
}).Then([](Foo foo){
  // code
});
```

#### tasks queue (unpacking tuple)
```CPP
PostTask(holder, []() {
  // code
  return std::make_tuple(Foo{}, Bar{});
}).Then([](Foo foo, Bar bar){
  // code
});
```
