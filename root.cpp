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
            registry.push_back(&ptr);
        }
        root(T* const other_ptr): ptr(other_ptr) {
            garbage_collector->add_to_registry(&ptr);
            registry.push_back(&ptr);
        }
        
        T* get_ptr() const {
            return ptr;
        }

        T& operator*() const {
            if(ptr == nullptr){
                throw std::logic_error("Cannot dereference a null pointer!");
            }
            
            return *ptr;
        }

        T* operator->() const {
            if(ptr == nullptr){
                throw std::logic_error("Cannot dereference a null pointer!");
            }
            
            return ptr;
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
            
            return *(ptr + i);
        }
        
    private:
        T *ptr;
};

// void print_registry(){
//     std::vector<void *> reg;
//     for(int i = 0; i < reg.size(); ++i){
//         // root<T>& r = *reg[i];
//         if(r != nullptr){
//             std::cout << *r << std::endl;
//         }
//     }
// }

void root_test1(){
    struct t {
        int a;
        char b;
        bool c;
        t(int a, char b, bool c): a(a), b(b), c(c) {}
    };
    root<struct t> r = new struct t(1, 'a', true);
    cout << (*r).a << endl << r->b << endl;
}

void root_test2(){
    root<int> a = new int [10];
    for(int i = 0; i < 10; ++i){
        a[i] = i+1;//*(a+i)
        cout << a[i] << endl;
    }
    
    print_registry<int>();
    cout << a[5] << endl;
    a = new int;
    *a = 3;
    cout << *a << std::endl;

}

void root_test3(){{
    root<int> a = new int(6);
    // a = nullptr;
    root<int> b = new int(2);{
    root<int> bb = b;

    print_registry();

}
    
    print_registry();
    
    
}
root<int> b = new int(4);

print_registry();
    
}