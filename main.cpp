#include <iostream>
#include "collector.hpp"
#include "root.hpp"
using std::cout;
using std::endl;

int main(){
    // int *a = new int;
    // a[0] = 5;
    // std::cout << a[0] << std::endl;
    // root<void> b;
    // b = gc.allocate<int>();
    // int *c = (int *)b.get_ptr();
    gc::root<void> cc;
    gc::detail::collector &gc = gc::detail::collector::instance();
    switch (gc.mode) {
        case gc::collection_mode::Manual:   cout << "Manual" << endl; break;
        case gc::collection_mode::Normal:  cout << "Normal" << endl; break;
        case gc::collection_mode::Stress:  cout << "Stress" << endl;
    }
    int* a;
    {
        gc::root<int> b;
        b = gc::allocate<int>();
        *b = 5;
        a = b.get_ptr();
        cout << *a << endl;
    }
    // gc.collect();
    cout << *a << endl;
    *a = 6;
    cout << *a << endl;
    // gc::detail::collector::instance().get
    // cout << *c << endl;
}
