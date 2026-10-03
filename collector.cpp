#include <algorithm>
#include <stdexcept>
#include <vector>
#include "collector.hpp"

namespace gc {
    namespace detail {
        collector& collector::instance() {
            static collector garbage_collector;
            return garbage_collector;
        }

        bool collector::is_marked(void *ptr){
            if(metadata.find(ptr) != metadata.end()) {
                return metadata[ptr].marked;
            }
            else {
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

            std::vector<void*> children;
            while(!mark_stack.empty()){
                void* obj = mark_stack.back();
                mark_stack.pop_back();
                children.clear();
                metadata[obj].trace(children);
                for(void* child: children) {
                    // If the child has allocated something and it's not marked already
                    if(metadata.find(child) != metadata.end() && !metadata[child].marked){
                        metadata[child].marked = true;
                        mark_stack.push_back(child);
                    }
                }
            }
        }


        void collector::sweep(){
            for(auto it = metadata.begin(); it != metadata.end();) {
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


        void collector::collect_if_needed() {
            switch (cfg.mode) {
                case collection_mode::Manual:
                    break;
                case collection_mode::Stress:
                    collect();
                    break;
                case collection_mode::Normal:
                    if(heap_bytes >= cfg.next_gc){
                        collect();
                        cfg.next_gc = std::max(static_cast<size_t>(heap_bytes * cfg.growth_factor), MB);
                    }
                    break;
            }
        }

        void collector::configure(const config& c) {
            if (!(c.growth_factor > 1.0 && c.growth_factor < 100.0)){ // NaN is rejected too
                throw std::invalid_argument("growth_factor must be > 1 and < 100");
            }
            if (c.next_gc <= heap_bytes){
                throw std::invalid_argument("next_gc must be greater than heap_bytes");
            }
            cfg.mode = c.mode;
            cfg.growth_factor = c.growth_factor;
            cfg.next_gc = std::max(c.next_gc, MB);
        }
    }
    
    void collect() {
        detail::collector::instance().mark();
        detail::collector::instance().sweep();
    }

    config get_config(){
        return detail::collector::instance().get_config();
    }

    void configure(const config& c){
        detail::collector::instance().configure(c);
    }
}
