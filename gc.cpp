#include <iostream>
#include <vector>
#include <unordered_map>
#include <functional>
#include <type_traits>
using std::cout;
using std::endl;

// consider the allocation metadata struct being the header of each allocation. and not use the hashmap.
// You allocate sizeof(header) + sizeof(T) and then return pointer + sizeof(header).
// If you need the metadata pointer - sizeof(header).
// This way you avoid the overhead of the hashmap.
// What will you put in the header:
// same stuff as the hashmap. marked, trace, deallocate, print_allocation.
// also a pointer to the next allocation. This way you can traverse all allocations.
// also a pointer to the previous allocation. This way you can remove an allocation from the doubly linked list in O(1) time.

// Maybe do that after you have a working version with the hashmap.



class collector{
    public:
        template <typename T>
        // maybe we want a way to add args later for the allocation
        T *allocate(int array_size = 0) {
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
            alloc.trace = [array_size, ptr]() {
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

            
            alloc.deallocate = [array_size, ptr]() { // previously it was passing a void* and casting that to a T. i think that was a worse approach
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
                const int loop_size = array_size == 0 ? 1 : array_size;
                cout << "Allocation contents:" << endl;
                for(int i = 0; i < loop_size; ++i){
                    cout << ptr[i] << ' ';
                }cout << endl;
            };
            
            metadata[ptr] = alloc;

            return ptr;
        }


    private:
        struct allocation {// subject to change
            bool marked;
            // room for improvement here: don't pass the entire array of pointers. Give them out one by one.
            std::function<std::vector<void *>(void)> trace;
            std::function<void(void)> deallocate;
            std::function<void(void)> print_allocation;
            // FOR NOW WE WILL USE STD::FUNCTION!!!
            // possibly don't use function <> and instead use a template or a function pointer. They are more efficient.
            // also if you move stuff arount the captured variables will be invalidated.
        };        

        std::unordered_map<void *, struct allocation> metadata;

        // temporary
        friend int main();
};

int main(){
    collector gc;
    int **p = gc.allocate<int*>();
    int **q = gc.allocate<int*>();
    *p = gc.allocate<int>();
    *q = gc.allocate<int>();
    **p = 5;
    **q = 4;
    
    cout << " pointer values: "<< endl;
    cout << p << ' ' << *p << ' ' << **p << endl;
    cout << q << ' ' << *q << ' ' << **q << endl;
    cout << endl;
    

    for(auto &[_, b]: gc.metadata){
        b.print_allocation();
        cout << endl;
    }
    for(auto &[_, b]: gc.metadata){
        std::vector<void *> ch = b.trace();
        cout << "Number of pointers following: " << ch.size() << endl;
        for(int i = 0; i < ch.size(); ++i){
            cout << *(int *)(ch[i]) << endl;
        }
        cout << endl;
    }
}
