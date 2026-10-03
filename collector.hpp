#pragma once
#include <vector>
#include <unordered_map>
#include <type_traits>
#include <cstddef>
#include <functional>
#include "types.hpp"

namespace gc {
    namespace detail {
        class collector{
            private:
                struct allocation {
                    bool marked;
                    std::function<void(std::vector<void*> &)> trace;
                    std::function<size_t(void)> deallocate;
                };

                collector() = default;
                ~collector() = default;

            public:
                // returns a reference to the singleton instance of the collector
                static collector& instance();

                collector(const collector &) = delete;
                collector &operator=(const collector &) = delete;
                
                template <typename T>
                T *allocate_raw(size_t array_size = 0);
                
                void mark();
                void sweep();
                bool is_marked(void *ptr);
                
                // manual mode: does nothing
                // normal mode: collects if allocated bytes exceed some threshold
                // stress mode: collects
                void collect_if_needed();
                
                const std::unordered_map<void*, allocation>& get_metadata(){ return metadata; }
                const std::vector<void**>& get_registry(){ return registry; }
                void add_to_registry(void** ptr_to_root_ptr){ registry.push_back(ptr_to_root_ptr); }
                void remove_from_registry(){ registry.pop_back(); }
                
                // collection parameters for normal mode
                size_t get_heap_bytes() { return heap_bytes; }
                size_t get_next_gc() { return next_gc; }
                void set_next_gc(std::size_t bytes);
                double get_growth_factor() { return growth_factor; }
                void set_growth_factor(double factor);
                
                collection_mode mode = collection_mode::Normal;
                static constexpr size_t default_next_gc = MB;
                static constexpr double default_growth_factor = 2;
            private:
                std::unordered_map<void*, allocation> metadata;
                std::vector<void**> registry;

                size_t heap_bytes = 0;
                size_t next_gc = default_next_gc;
                double growth_factor = default_growth_factor;
        };

        template <typename T>
        T* collector::allocate_raw(size_t array_size) {
            // maybe break down this function.
            collect_if_needed(); // move this somewhere else?

            T *ptr;
            if(array_size == 0) {
            ptr = new T();
            }
            else {
                ptr = new T[array_size]();
            }

            heap_bytes += (array_size == 0 ? 1 : array_size) * sizeof(T);

            allocation alloc;
            alloc.marked = false;
            
            alloc.trace = [array_size, ptr](std::vector<void*> &children) -> void {
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
    }
    
    template <typename T>
    T* allocate(size_t array_size = 0) {
        return detail::collector::instance().allocate_raw<T>(array_size);
    }

    void collect();
}
