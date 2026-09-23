#include <iostream>
#include "collector.h"
#include "root.h"

int main(){
    // int *a = new int;
    // a[0] = 5;
    // std::cout << a[0] << std::endl;
    root<void> b;
    collector &gc = *b.get_garbage_collector();
    // b = gc.allocate<int>();
    // int *c = (int *)b.get_ptr();
    // cout << *c << endl;
}
