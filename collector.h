#pragma once
#include <iostream>
#include <vector>
#include <unordered_map>
#include <functional>
#include <type_traits>
#include <cstddef>
#include <stdexcept>
#include <algorithm>
using std::cout;
using std::endl;
// also we have a tiny problem: When a root is created this automatically creates a garbage collector
// but he is never deallocated
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
        // maybe those 2 should be inside of private??? breaks testing though
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
            // room for improvement here: don't pass the entire array of pointers. Give them out one by one.
            std::function<std::vector<void *>(void)> trace;
            std::function<size_t(void)> deallocate;
            // std::function<void(void)> print_allocation;
            // could i make the lambdas normal functions?
            // FOR NOW WE WILL USE STD::FUNCTION!!!
            // possibly don't use function <> and instead use a template or a function pointer. They are more efficient. try the template first.
            // also if you move stuff around the captured variables will be invalidated.
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
            // some kind of bug here?
            // Well yes but not really
            // In case the root object isn't on the stack then this doesn't work we have a bug, but for now it works
        }

    private:
        size_t heap_bytes = 0;
        size_t next_gc = MB; // when heap_bytes reaches next_gc marking and sweeping happens
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
// maybe we want a way to add args later for the allocation
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
    
    // you get the vector of void pointers during the marking phase and if they exist inside the hash map you follow them
    // otherwise you ignore them
    // forget array of void * wrong approach. we will create the void * each time in a trace function
    // and pass it on to the marker each time.
    alloc.trace = [array_size, ptr]() -> std::vector<void *>{
        // We initialize as void * because an object can push many kinds of pointers in here not only T!
        std::vector<void *> children;
        if constexpr (std::is_scalar_v<T> && !std::is_pointer_v<T>) {
            // primitive or enum — nothing to trace
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

    
    alloc.deallocate = [array_size, ptr]() -> size_t{ // previously it was passing a void* and casting that to a T. i think that was a worse approach
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
