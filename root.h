#pragma once
#include <iostream>
#include <vector>
#include "collector.h"
using std::cout;
using std::endl;

// maybe not a good design fix later.
class root_base {
    protected:
        inline static collector *garbage_collector = nullptr;
    public:
        root_base() {
            if(garbage_collector == nullptr){
                garbage_collector = new collector();
            }
        }

        static collector* get_garbage_collector(){
            if(garbage_collector == nullptr){
                throw std::logic_error("Garbage collector not initialized!");
            }
            return garbage_collector;
        }
};

template<typename T>
class root : public root_base {
    public:
        root(): root_base(), ptr(nullptr) {
            garbage_collector->add_to_registry(&ptr);
        }
        ~root() {
            garbage_collector->remove_from_registry();
        }
        
        
        root(const root &other_root): root_base(), ptr(other_root.get_ptr()) {
            garbage_collector->add_to_registry(&ptr);
        }
        root(T* const other_ptr): root_base(), ptr(other_ptr) {
            garbage_collector->add_to_registry(&ptr);
        }
        
        T* get_ptr() const {
            return static_cast<T*>(ptr);
        }

        template<typename U = T>
        U& operator*() const {
            if(ptr == nullptr){
                throw std::logic_error("Cannot dereference a null pointer!");
            }

            return *static_cast<U*>(ptr);
        }

        T* operator->() const {
            if(ptr == nullptr){
                throw std::logic_error("Cannot dereference a null pointer!");
            }
            
            return static_cast<T*>(ptr);
        }
        
        const root& operator=(T* const other_ptr) {
            ptr = other_ptr;
            return *this;
        }
        const root& operator=(const root &other_root) {
            ptr = other_root.get_ptr();
            return *this;
        }

        bool operator==(const root &other_root) const {
            return ptr == other_root.get_ptr();
        }

        bool operator==(T* const other_ptr) const {
            return ptr == other_ptr;
        }

        template <typename U>
        bool operator!=(const U &other) const {
            return !(*this == other);
        }

        template<typename U = T>
        U& operator[](int i) const {
            if(ptr == nullptr){
                throw std::logic_error("Cannot dereference a null pointer!");
            }

            return *(static_cast<U*>(ptr) + i);
        }
        
    private:
        void *ptr;
        
};
