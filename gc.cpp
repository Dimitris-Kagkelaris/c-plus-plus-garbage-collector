#include <iostream>
#include <vector>
#include <unordered_map>
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

            struct allocation alloc();
            
            metadata[ptr] = alloc;
                


            return ptr;
        }


    private:
        struct allocation {// subject to change
            bool marked;
            void trace(void *);
            void deallocate(void *);
            // maybe use function<> something instead of function pointers
            allocation(void *t(void *), void *d(void *)){
                marked = false;
                trace = t;
                deallocate = d;
            }
        };

        std::unordered_map<void*, struct allocation> metadata;
};