#pragma once
#include <stdexcept>
#include "collector.hpp"

namespace gc {
    template<typename T>
    class root {
        private:
            void* ptr;

        public:
            root(): ptr(nullptr) {
                detail::get_collector().add_to_registry(&ptr);
            }
            ~root() {
                detail::get_collector().remove_from_registry();
            }
            
            root(const root &other_root): ptr(other_root.get_ptr()) {
                detail::get_collector().add_to_registry(&ptr);
            }
            root(T* const other_ptr): ptr(other_ptr) {
                detail::get_collector().add_to_registry(&ptr);
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
    };
}
