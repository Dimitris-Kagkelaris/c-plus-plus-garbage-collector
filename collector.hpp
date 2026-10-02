#pragma once
#include <iostream>
#include <vector>
#include <unordered_map>
#include <functional>
#include <type_traits>
#include <cstddef>
#include <stdexcept>
#include <algorithm>
#include "models.hpp"

class collector{
    public:
        collector(collection_mode m = collection_mode::Normal): mode(m) {}
        collector(const collector &) = delete;
        collector &operator=(const collector &) = delete;

        template <typename T>
        T *allocate_raw(size_t array_size = 0);

        void mark();
        void sweep();
        void collect_if_needed();
        bool is_marked(void *ptr);
        
        std::unordered_map<void *, struct allocation> get_metadata(){ return metadata; }

        std::vector<void**> get_registry(){ return registry; }
        void add_to_registry(void **ptr_to_root_ptr){ registry.push_back(ptr_to_root_ptr); }
        void remove_from_registry(){ registry.pop_back(); }

        size_t get_heap_bytes() { return heap_bytes; }
        size_t get_next_gc() { return next_gc; }
        void set_next_gc(std::size_t bytes);
        double get_growth_factor() { return growth_factor; }
        void set_growth_factor(double factor);
        
        collection_mode mode;
    private:
        std::unordered_map<void*, struct allocation> metadata;
        std::vector<void**> registry;
        
        size_t heap_bytes = 0;
        size_t next_gc = MB;
        double growth_factor = 2;
};

collector& get_collector();

template <typename T>
T* collector::allocate_raw(size_t array_size) {
    // maybe break down this function.
    collect_if_needed();

    T *ptr;
    if(array_size == 0) {
       ptr = new T();
    }
    else {
        ptr = new T[array_size]();
    }

    heap_bytes += (array_size == 0 ? 1 : array_size) * sizeof(T);

    struct allocation alloc;
    alloc.marked = false;
    
    alloc.trace = [array_size, ptr]() -> std::vector<void *>{
        std::vector<void *> children;
        if constexpr (std::is_scalar_v<T> && !std::is_pointer_v<T>) {
            // primitive or enum. Nothing to trace
        }
        else{
            const int loop_size = array_size == 0 ? 1 : array_size;
            for(int i = 0; i < loop_size; ++i){
                if constexpr (std::is_pointer_v<T>) {
                    children.push_back(ptr[i]);
                }
                else {
                    ptr[i].trace(children);
                }
            }
        }
        return children;
    };

    
    alloc.deallocate = [array_size, ptr]() -> size_t {
        if(array_size == 0) {
            delete ptr;
        }
        else {
            delete[] ptr;
        }
        return (array_size == 0 ? sizeof(T) : array_size * sizeof(T));
    };
    
    metadata[ptr] = alloc;

    return ptr;
}

template <typename T>
T* allocate(size_t array_size = 0) {
    return get_collector().allocate_raw<T>(array_size);
}

void collect(){
    get_collector().mark();
    get_collector().sweep();
}
