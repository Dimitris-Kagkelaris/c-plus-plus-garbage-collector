#pragma once
#include <vector>
#include <unordered_map>
#include <type_traits>
#include <cstddef>
#include <functional>
#include "types.hpp"

namespace gc {
    // collection parameters (threshold and growth factor only matter in normal mode)
    struct config {
        collection_mode mode;
        size_t next_gc;
        double growth_factor;
        config(collection_mode m = collection_mode::Normal, size_t n = default_next_gc, double g = default_growth_factor)
            : mode(m), next_gc(n), growth_factor(g) {}
    };

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
                
                size_t get_heap_bytes() { return heap_bytes; }
                const config& get_config(){ return cfg; }
                void configure(const config& c);

            private:
                std::unordered_map<void*, allocation> metadata;
                std::vector<void**> registry;
                
                config cfg;
                size_t heap_bytes = 0;
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
            

            if constexpr (std::is_scalar_v<T> && !std::is_pointer_v<T>) {
                // primitive or enum. Nothing to trace
                alloc.trace = [](std::vector<void*> &) {};
            }
            else {
                alloc.trace = [array_size, ptr](std::vector<void*> &children) {
                    const size_t loop_size = array_size == 0 ? 1 : array_size;
                    for(size_t i = 0; i < loop_size; ++i){
                        if constexpr (std::is_pointer_v<T>) {
                            children.push_back(ptr[i]);
                        }
                        else {
                            ptr[i].trace(children);
                        }
                    }
                };
            }

            
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
    
    // returns the current configuration of the collector
    config get_config();
    // validates and applies the configuration to the collector
    void configure(const config& c);
}
