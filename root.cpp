#include <iostream>
#include <vector>
#include "collector.h"
using std::cout;
using std::endl;

template<typename T>
class root {
    // maybe add const roots later
    public:
        root(): ptr(nullptr) {
            garbage_collector->add_to_registry(&ptr);
        }
        ~root() {
            garbage_collector->remove_from_registry();
        }
        
        
        root(const root &other_root): ptr(other_root.get_ptr()) {
            garbage_collector->add_to_registry(&ptr);
        }
        root(T* const other_ptr): ptr(other_ptr) {
            garbage_collector->add_to_registry(&ptr);
        }
        
        T* get_ptr() const {
            return static_cast<T*>(ptr);
        }

        // should disable this operator in case someone creates a Root<void> and maybe some other operators as well
        T& operator*() const {
            if(ptr == nullptr){
                throw std::logic_error("Cannot dereference a null pointer!");
            }
            
            return *static_cast<T*>(ptr);
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

        T& operator[](int i) const {
            if(ptr == nullptr){
                throw std::logic_error("Cannot dereference a null pointer!");
            }
            
            return *(static_cast<T*>(ptr) + i);
        }
        
    private:
        void *ptr;
        
};
