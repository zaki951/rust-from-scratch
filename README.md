# Learning Rust: A Personal Journey
This repository documents my learning journey with the Rust programming language.
It includes two projects that reflect different stages in my understanding of systems programming and Rust concepts.

## 🧪 Project 1: A Minimal TCP Implementation in C++
The first project is a lightweight TCP implementation in C++, inspired by [jonhoo/rust-tcp](https://github.com/jonhoo/rust-tcp), an educational project originally written in Rust. 

At that point, I was just beginning to explore Rust and found it challenging to read and understand.
Reimplementing the project in C++ helped me bridge the gap between what I already knew and what I was trying to learn.
It allowed me to recognize shared underlying principles by comparing the syntax and structure of both languages.

Why start with TCP?
Networking is often a great entry point when learning a systems language. It touches on key areas like:

* Multithreading

* System calls and low-level I/O

* Parsing and binary protocols

* Error handling and resource management

The goal wasn’t to write a complete or production-grade TCP stack in C++.
Instead, I used C++ as a stepping stone to understand Rust more clearly.
That’s also why the project doesn’t include a full build system or unit tests. The focus was on experimentation and learning, not engineering best practices.

## 🦀 Project 2: Codecrafters Interpreter in Rust
Once I became more confident reading Rust code, I started building small programs and eventually took on the [Codecrafters interpreter challenge](https://app.codecrafters.io/courses/interpreter/overview).

I completed 67 out of 84 stages. By then, I had achieved what I set out to do: gain a practical understanding of Rust syntax, semantics, and interpreter design.
I chose to stop at that point, as continuing further wouldn't significantly deepen my learning.

## 🚀 Why Learn Rust?
Safety is a critical concern in software development, especially at the systems level.
Languages like C and C++ are powerful and capable of building core infrastructure such as operating systems, but they offer few built-in safeguards. Mistakes can easily lead to undefined behavior, crashes, or security issues.

While there are ways to improve safety in C++, like:

* Enabling compiler warnings and treating them as errors

* Using tools like AddressSanitizer or static analysis

These solutions are optional, and developers must enforce them manually.

Rust takes a different approach, it integrates safety directly into the language.
With its ownership system, borrowing rules, strict compile-time checks, and built-in concurrency safety, Rust helps developers write robust, reliable code without sacrificing performance.

What I find especially exciting is that Rust not only offers practical solutions but also influences other languages to evolve in the same direction.

By rewriting systems code in Rust, developers can reduce debugging time, and organizations can improve stability and lower maintenance costs.

