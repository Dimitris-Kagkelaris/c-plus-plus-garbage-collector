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

// Add support for other types other than primitives?

void collector::mark(){
    // do dfs maybe later this will become incremental and interruptable
    std::vector<void*> mark_stack;//.reserve? we use vector instead of stack for performance

    for(void** root_ptr: registry){
        if(metadata.find(*root_ptr) != metadata.end()){
            void* obj = *root_ptr;
            mark_stack.push_back(obj);
            metadata[obj].marked = true;
        }
    }

    while(!mark_stack.empty()){
        void* obj = mark_stack.back();
        mark_stack.pop_back();
        std::vector<void *> children = metadata[obj].trace();
        for(void* child: children){
            // If the child has allocated something and it's not marked already
            if(metadata.find(child) != metadata.end() && !metadata[child].marked){
                metadata[child].marked = true;
                mark_stack.push_back(child);
            }
        }
    }
}


void collector::sweep(){
    for(auto it = metadata.begin(); it != metadata.end();){
        if(it->second.marked){
            it->second.marked = false;
            ++it;
        }
        else{
            // this needs to become atomic in case we allow sweeping and running the user program concurently
            heap_bytes -= it->second.deallocate();
            it = metadata.erase(it);
        }
    }
}
