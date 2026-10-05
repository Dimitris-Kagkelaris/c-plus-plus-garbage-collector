#include <iostream>
#include <vector>
#include <cplusplusgc.hpp>
using std::cout;
using std::endl;

// every class type allocated with gc::allocate needs a trace function that reports its GC pointers
struct node {
    int value = 0;
    node* next = nullptr;

    ~node() { cout << "freed node " << value << endl; }

    void trace(std::vector<void*> &children){
        children.push_back(next);
    }
};

struct blob {
    static inline int freed = 0;
    int data[4] = {};

    ~blob() { ++freed; }

    void trace(std::vector<void*> &){}
};

int main(){
    gc::configure(gc::config(gc::collection_mode::Manual)); // collect only on gc::collect()

    gc::root<node> head = gc::allocate<node>();
    head->value = 1;
    head->next = gc::allocate<node>();
    head->next->value = 2;

    gc::root<int> numbers = gc::allocate<int>(5);
    for(int i = 0; i < 5; ++i){
        numbers[i] = i * i;
    }

    {
        gc::root<node> temp = gc::allocate<node>();
        temp->value = 3;
    }

    gc::collect(); // free node 3
    cout << "list: " << head->value << " -> " << head->next->value << endl;
    cout << "numbers[4] = " << numbers[4] << endl;

    head->next = nullptr;
    gc::collect(); // free node 2


    gc::configure(gc::config(gc::collection_mode::Normal)); // collects once the heap passes a threshold or on gc::collect()
    for(int i = 0; i < 1'000'000; ++i){
        gc::allocate<blob>();
    }
    cout << "blobs freed without calling collect(): " << blob::freed << endl;
    // the rest wait for the next collection
}
