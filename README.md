# Garbage Collector for C++

A precise mark and sweep garbage collector library for C++17. Objects are allocated through the garbage collector, kept alive by explicit roots, and freed automatically once nothing reachable points to them.

```cpp
gc::root<int> p = gc::allocate<int>();
*p = 1;

gc::root<int> numbers = gc::allocate<int>(5);
for(int i = 0; i < 5; ++i){
    numbers[i] = i * i;
}

{
    gc::root<int> q = gc::allocate<int>();
    *q = 7;
}

gc::collect(); // 7 is freed, 1 and numbers survive
```

See [`example/example.cpp`](example/example.cpp) for a complete program, including a traced struct and the collection modes.

## Features

* **Precise** - only pointers that objects report through `trace()` are followed.

* **Collection modes** - collect when the heap passes a threshold (`Normal`), before every allocation (`Stress`), or only on `gc::collect()` (`Manual`).

* **Works with raw pointers** - pointers to memory the collector didn't allocate are ignored, so GC objects can also hold regular `new`'d memory.

## Usage

### Allocating and rooting

| API                     | Description                                                                    |
| ----------------------- | ------------------------------------------------------------------------------ |
| `gc::allocate<T>()`     | Allocates a single value-initialized `T` and returns a `T*`.                   |
| `gc::allocate<T>(n)`    | Allocates an array of `n` value-initialized `T`s (`n > 0`) and returns a `T*`. |
| `gc::root<T>`           | A pointer wrapper that keeps the object it points to alive.                    |
| `gc::collect()`         | Runs a full collection immediately.                                            |
| `gc::get_config()`      | Returns the current collector configuration.                                   |
| `gc::configure(config)` | Validates and applies a new configuration.                                     |

A `gc::root<T>` supports `*`, `->`, `[]`, `==` / `!=` (against roots or raw pointers) and assignment from a `T*` or another root. The raw pointer is available through `get_ptr()`. Dereferencing a null root throws `std::logic_error`.

### Traceable types

Every class type allocated with `gc::allocate` must have a `trace` member that pushes each GC pointer it holds:

```cpp
void trace(std::vector<void*> &children);
```

Primitive types, enums and pointers need nothing: `gc::allocate<int>()` has nothing to trace, and `gc::allocate<int*>()` traces its pointee automatically. For arrays, `trace` is called automatically on every element.

### Configuration

```cpp
gc::configure(gc::config(gc::collection_mode::Normal, 8 * gc::MB, 1.5));
```

| Field           | Default  | Description                                                                                                                                                  |
| --------------- | -------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `mode`          | `Normal` | **`Normal`** collects when the heap reaches `next_gc`. **`Stress`** collects before every allocation. **`Manual`** collects only on `gc::collect()`.          |
| `next_gc`       | 1 MiB    | Heap size in bytes that triggers the next collection. Values below 1 MiB are raised to 1 MiB. After each collection it becomes `max(live_bytes * growth_factor, 1 MiB)`. |
| `growth_factor` | `2.0`    | How much the threshold grows relative to the surviving heap after a collection.                                                                              |

In `Normal` mode, `configure` throws `std::invalid_argument` if `growth_factor` isn't strictly between 1 and 100, or if `next_gc` isn't greater than the current heap size. The other modes don't use these values, so they aren't checked there.

## Rules

The collector only knows what roots and `trace()` tell it. Breaking these rules is undefined behavior (typically a use after free).

* **Roots are local variables, destroyed in reverse order of creation.** Roots are registered on a stack, and each root removes the most recently registered entry when it is destroyed. Don't store roots in containers (`std::vector`, `std::optional`, ...), on the heap (`new`, `std::unique_ptr`), in `static` or `thread_local` variables, or as members of GC-allocated objects. Don't pass roots by value or return named roots; pass `T*` or `const root<T>&` instead.

* **Root a new object before the next allocation.** Any allocation can trigger a collection, so an object that isn't reachable from a root yet can be freed.

* **`trace()` must report every GC pointer.** An unreported pointer looks like garbage to the collector, and its object gets freed while still in use.

* **Destructors must not touch other GC objects.** Unreachable objects are destroyed in no particular order, so anything a destructor points to may already be freed. Destructors may still release resources the object owns itself, and they must not call `gc::allocate`.

* **Pointers must point to the start of an object.** A pointer into the middle of an array, to a member, or to a non-first base class doesn't keep the object alive.

* **Single-threaded only.** The collector is a global singleton with no synchronization.

## Building

Requires a C++17 compiler and, for the CMake options, CMake 3.21 or newer. Place the library in your project directory, then use one of the following:

* **CMake subdirectory** (recommended):

  ```cmake
  add_subdirectory(cppgc cppgc_build)
  target_link_libraries(my_app PRIVATE cppgc)
  ```

* **Prebuilt static library:**

  ```bash
  cmake -S cppgc -B cppgc/_build -DCMAKE_BUILD_TYPE=Release
  cmake --build cppgc/_build
  g++ -std=c++17 main.cpp -Icppgc -Icppgc/include -Lcppgc/_build -lcppgc -o my_app
  ```

* **Compile the sources directly:**

  ```bash
  g++ -std=c++17 main.cpp cppgc/src/*.cpp -Icppgc -Icppgc/include -o my_app
  ```

Then `#include <cppgc.hpp>`.

## Development

Tests use [doctest](https://github.com/doctest/doctest). The `build` script configures, builds and runs them:

* **`./build release`** - optimized build.

* **`./build debug`** - Debug build with AddressSanitizer and UndefinedBehaviorSanitizer.

* **`./build leakcheck`** - Debug build checked with macOS `leaks`. macOS only.

* **`./build clean`** - removes the build directories.

All library code compiles with `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wcast-align`. The example builds with `example/build-example`.


## Limitations

* **Default-constructible types only** - `gc::allocate` uses `new T()`, so constructor arguments can't be passed.

* **No cleanup at exit** - like most garbage collectors, objects still alive when the program exits aren't destroyed. The OS reclaims the memory, but their destructors don't run.

* **Metadata overhead** - Each allocation also costs a hash map node and two `std::function` objects.

* **Tested platforms** - Tested with Apple Clang on macOS and GCC on Linux. The library is standard C++17, but the build scripts require bash, and the sanitizer and warning flags assume GCC or Clang.

## Future Work

* Roots that can live anywhere: in containers, on the heap, and passed by value.
* Replacing the metadata hash map with a linked list.

## Author

**Dimitrios Kagkelaris**<br>
GitHub: [Dimitris-Kagkelaris](https://github.com/Dimitris-Kagkelaris)
