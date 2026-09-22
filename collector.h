#pragma once
#include <iostream>
#include <vector>
#include <unordered_map>
#include <functional>
#include <type_traits>
using std::cout;
using std::endl;
class collector{
    public:
        collector(){}
        collector(const collector &) = delete;
        collector &operator=(const collector &) = delete;

        template <typename T>
        T *allocate(int array_size = 0);

        void mark(){
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


        void sweep(){
            for(auto it = metadata.begin(); it != metadata.end();){
                if(it->second.marked){
                    it->second.marked = false;
                    ++it;
                }
                else{
                    it->second.deallocate();
                    it = metadata.erase(it);
                }
            }
        }
    private:
        struct allocation {
            bool marked; // subject to change
            // room for improvement here: don't pass the entire array of pointers. Give them out one by one.
            std::function<std::vector<void *>(void)> trace;
            std::function<void(void)> deallocate;
            std::function<void(void)> print_allocation;
            
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
        }
        //debugging, prints addresses of pointers to objects
        void print_registry(){
            cout << "printing registry:" << endl;
            for(size_t i = 0; i < registry.size(); ++i){
                std::cout << *registry[i] << std::endl;
            }
        }
};

template <typename T>
// maybe we want a way to add args later for the allocation
T* collector::allocate(int array_size) {
    T *ptr;
    if(array_size == 0) {
       ptr = new T();
    }
    else {
        ptr = new T[array_size]();
    }

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

    
    alloc.deallocate = [array_size, ptr]() -> void{ // previously it was passing a void* and casting that to a T. i think that was a worse approach
        if(array_size == 0) {
            delete ptr;
        }
        else {
            delete[] ptr;
        }
    };

    // for debugging:
    // works only for primitives and arrays for now
    alloc.print_allocation = [array_size, ptr]() {
        if constexpr (std::is_scalar_v<T>) {
            const int loop_size = array_size == 0 ? 1 : array_size;
            cout << "Allocation contents:" << endl;
            for(int i = 0; i < loop_size; ++i){
                cout << ptr[i] << ' ';
            }cout << endl;
        }
        else{
            // ptr[i].print_allocation();
        }
    };
    
    metadata[ptr] = alloc;

    return ptr;
}
