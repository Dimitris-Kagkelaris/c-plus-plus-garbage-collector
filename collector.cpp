// #include <iostream>
// #include <vector>
// #include <unordered_map>
// #include <functional>
// #include <type_traits>
#include "collector.h"
#include "root.h"
using std::cout;
using std::endl;

// consider the allocation metadata struct being the header of each allocation. and not use the hashmap.
// You allocate sizeof(header) + sizeof(T) and then return pointer + sizeof(header).
// If you need the metadata pointer - sizeof(header).
// This way you avoid the overhead of the hashmap.
// What will you put in the header:
// same stuff as the hashmap. marked, trace, deallocate, print_allocation.
// also a pointer to the next allocation. This way you can traverse all allocations.
// Singly linked list is better.

// Maybe do that after you have a working version with the hashmap.
