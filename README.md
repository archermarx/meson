# Quadratic

![Tests](https://github.com/archermarx/quadratic/actions/workflows/test.yml/badge.svg)

Robust C++ quadratic equation solver based on "The Ins and Outs of Solving Quadratic Equations with Floating-Point Arithmetic" by Frédéric Goualard.
Based on original Julia implementation at https://github.com/goualard-f/QuadraticEquation.jl.

## Installation

Download the [header file](https://github.com/archermarx/quadratic/include/header.h) and compile with your project using your preferred build system. Then, simply `#include "quadratic.h"`

## Usage

To solve a quadratic equation with parameters `a`, `b`, and `c`, call the `solve_quadratic` function. This function returns a ```std::pair<T, T>```, where `T` is the input type.
If there are no real solutions, then the pair will contain two NaNs. If there is only one solution, then it will be contained in the first element, while the second element of the pair will be NaN.

```cpp
  double a = 1.0;
  double b = 1.0;
  double c = 1.0;
  auto [x1, x2] = solve_quadratic(a, b, c);
```

