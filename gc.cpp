#include <iostream>
#include <vector>
#include <unordered_map>
#include <functional>
#include <type_traits>
using std::cout;
using std::endl;


class collector{
    public:
        template <typename T>
        // maybe we want a way to add args later for the allocation
        T *allocate(int array_size = 0) {
            T *ptr;
            if(array_size == 0) {
               ptr = new T;
            }
            else {
                ptr = new T[array_size];
            }

            struct allocation alloc;
            alloc.marked = false;
            
            // you get the vector of void pointers during the marking phase and if they exist inside the hash map you follow them
            // otherwise you ignore them
            if constexpr (std::is_scalar_v<T> && !std::is_pointer_v<T>) {
                // primitive or enum — nothing to trace
            }
            else{
                const int loop_size = array_size == 0 ? 1 : array_size;
                for(int i = 0; i < loop_size; ++i){
                    if constexpr (std::is_pointer_v<T>) {
                        alloc.children.push_back(ptr[i]);
                    }
                    else {
                        ptr[i].trace(alloc.children);
                    }
                }
            }
            
            alloc.deallocate = [array_size](void *p) {
                if(array_size == 0) {
                    delete static_cast<T*>(p);
                }
                else {
                    delete[] static_cast<T*>(p);
                }
            };
            
            metadata[ptr] = alloc;

            return ptr;
        }


    private:
        struct allocation {// subject to change
            bool marked;
            // this might have been a bad idea because it stores copies not references to what's allocated
            std::vector<void *> children;
            std::function<void(void *)> deallocate;
        };        

        std::unordered_map<void*, struct allocation> metadata;

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
    
    cout << (p == q) << endl;
    cout << (*p == *q) << endl;
    cout << (**p == **q) << endl;
    cout << "End\n";
    cout << *p << endl;
    cout << *q << endl;
    cout << **p << endl;
    cout << **q << endl;
    cout << endl;
    
    for(auto &[_, b]: gc.metadata){
        cout << b.children.size() << endl;
        for(int i = 0; i < b.children.size(); ++i){
            cout << *(int *)(b.children[i]) << endl;
        }
    }
}
