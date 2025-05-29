# Rust Interpreter

This is a Rust-based implementation of a Lox language interpreter, developed as part of the [Codecrafters Interpreter Challenge](https://app.codecrafters.io/courses/interpreter/overview).  
The goal of this project is to build a fully functional interpreter from scratch, following the structure of the Lox language introduced in *Crafting Interpreters* by Bob Nystrom.

- ✅ **Progress:** 67 out of 84 stages completed.
- **Note:** This project is still under development and is **not optimized for performance** at this stage.
- **Testing:** Unit tests are run using the `codecrafters test` command.

## Features
### String concatenation
  ```
  var a = "Hello " + "World!"
  print a
  ```
### Call native Rust functions, such as the built-in `clock` function:
  ```
  var start = clock();
  foo();
  var end = clock();
  print end - start;
  ```
### Closure
  ```
  fun makeCounter() {
    var i = 0;
    fun count() {
      i = i + 1;
      print i;
    }
    return count;
  }
  var counter = makeCounter();
  counter(); // prints 1
  counter(); // prints 2
  ```              

## How to run the example

```bash
./run.sh run example.lox
```
