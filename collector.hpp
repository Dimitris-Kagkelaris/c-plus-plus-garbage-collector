#pragma once
#include <iostream>
#include <vector>
#include <unordered_map>
#include <functional>
#include <type_traits>
#include <cstddef>
#include <stdexcept>
#include <algorithm>
constexpr size_t MB = 1024*1024;

class collector{
    public:
        enum class collection_mode {
            Normal,     // collect when heap_bytes reaches next_gc
            Stress,     // collect after every allocation
            Manual      // collect only on explicit collect() calls
        };
        collection_mode mode;
        
        collector(collection_mode cmode = collection_mode::Normal): mode(cmode) {}
        collector(const collector &) = delete;
        collector &operator=(const collector &) = delete;

        template <typename T>
        T *allocate(size_t array_size = 0);
        void mark();
        void sweep();
        void collect(){
            mark();
            sweep();
        }
        void collect_if_needed();

    private:
        struct allocation {
            bool marked; // subject to change
            std::function<std::vector<void *>(void)> trace;
            std::function<size_t(void)> deallocate;
        };       
        
        std::unordered_map<void *, struct allocation> metadata;
        std::vector<void **> registry;
        
    public:
        std::unordered_map<void *, struct allocation> get_metadata(){
            return metadata;
        }

        bool isMarked(void *ptr){
            if(metadata.find(ptr) != metadata.end()){
                return metadata[ptr].marked;
            }
            else{
                throw std::logic_error("Pointer not found in metadata!");
            }
        }

        std::vector<void **> get_registry(){
            return registry;
        }
        void add_to_registry(void **ptr_to_root_ptr){
            registry.push_back(ptr_to_root_ptr);
        }
        void remove_from_registry(){
            registry.pop_back();
        }

    private:
        size_t heap_bytes = 0;
        size_t next_gc = MB;
        double growth_factor = 2;
    
    public:

        size_t get_heap_bytes() { return heap_bytes; }
        size_t get_next_gc() { return next_gc; }
        double get_growth_factor() { return growth_factor; }

        void set_growth_factor(double factor) {
            if (!(factor > 1.0 && factor < 100.0)){ // NaN is rejected too
                throw std::invalid_argument("growth_factor must be > 1 and < 100");
            }
            growth_factor = factor;
        }

        void set_next_gc(std::size_t bytes) {
            if (bytes <= heap_bytes){
                throw std::invalid_argument("next_gc must be greater than heap_bytes");
            }
            next_gc = std::max(bytes, MB);
        }
};

template <typename T>
T* collector::allocate(size_t array_size) {

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
