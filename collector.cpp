// #include <iostream>
// #include <vector>
// #include <unordered_map>
// #include <functional>
// #include <type_traits>
#include "collector.hpp"
#include "root.hpp"

collector& get_collector() {
    static collector gc;
    return gc;
}

bool collector::is_marked(void *ptr){
    if(metadata.find(ptr) != metadata.end()){
        return metadata[ptr].marked;
    }
    else{
        throw std::logic_error("Pointer not found in metadata!");
    }
}

void collector::mark(){
    // we use vector instead of stack for performance
    std::vector<void*> mark_stack; 

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
            heap_bytes -= it->second.deallocate();
            it = metadata.erase(it);
        }
    }
}

void collector::collect_if_needed(){
    switch (mode) {
        case collection_mode::Manual:
            break;
        case collection_mode::Stress:
            collect();
            break;
        case collection_mode::Normal:
            if(heap_bytes >= next_gc){
                collect();
                next_gc = std::max(static_cast<size_t>(heap_bytes * growth_factor), MB);
            }
            break;
    }
}

void collector::set_growth_factor(double factor) {
    if (!(factor > 1.0 && factor < 100.0)){ // NaN is rejected too
        throw std::invalid_argument("growth_factor must be > 1 and < 100");
    }
    growth_factor = factor;
}

void collector::set_next_gc(std::size_t bytes) {
    if (bytes <= heap_bytes){
        throw std::invalid_argument("next_gc must be greater than heap_bytes");
    }
    next_gc = std::max(bytes, MB);
}
