<!-- Ways to build after you put the library in your project dir
1) g++ -std=c++17 test.cpp cppgc/src/*.cpp -Icppgc -Icppgc/include -o app
2) cmake -S . -B build
    cmake --build build
    g++ -std=c++17 test.cpp -Icppgc -Icppgc/include -Lcppgc/build -lcppgc -o test

3) 

add_subdirectory(cppgc cppgc_build)

add_executable(test test.cpp)
target_link_libraries(test PRIVATE cppgc) -->

# Garbage Collector for C++

A precise mark and sweep garbage collector library for C++

### example (very small one i think)

<!-- syntax thingy that wraps code -->
gc::root<int> p = gc::allocate<int>();
*p = 1;

gc::root<int> numbers = gc::allocate<int>(5);
for(int i = 0; i < 5; ++i){
    numbers[i] = i * i;
}

{
    gc::root<int> q = gc::allocate<int>();
    *q = 7
}
// at collection time 1 will survive but 7 will get deleted
// look at example/example.cpp for a more detailed example
<!--  -->


## Architecture

The work is distributed across 2 core classes.

* **root<T>** — provides raw pointer wrapping allowing the keeping track of roots pointing to live objects in the heap.

* **collector** - provides the allocation and collection (mark & sweep) functionalities alongside configuration of the frequency of the allocation.

## Usage

* **gc::root<T>** to create the GC pointer
this supports most raw pointer functionalities

* **gc::allocate<T>()** to allocate an object of type T

* **gc::allocate<T>(n)** to allocate an array of objects of type T (n must be positive)

* **gc::collect()** to trigger an explicit collection

* **gc::get_config()** and **gc::configure(const config &c)** to tune the parameters of the collection. Parameters to be set are:

    * collection_mode: 
        * Normal (triggers collection when allocated number of bytes exceeds a threshold)
        * Stress (triggers collection after every allocation)
        * Manual (triggers collection only on gc::collect() calls)

    * next_gc: the threshold until next allocation. Defaults to 1 MiB and can be set to anything higher

    * growth_factor: the ratio by which next_gc is calculated after a collection. Specifically after collection: next_gc = live_heap_bytes * growth_factor

gc::get_config() returns a struct containing the current parameters and configure sets the new parameters according to the struct passed

## Building

The project uses CMake version ...

Suggested usage is to include the entire project in your working directory and 

1) add to CMake via add_subdirectory(cppgc cppgc_build) and target_link_libraries(some_project PRIVATE cppgc)

2) Without CMake g++ -std=c++17 some_project.cpp cppgc/src/*.cpp -Icppgc -Icppgc/include -o some_project




