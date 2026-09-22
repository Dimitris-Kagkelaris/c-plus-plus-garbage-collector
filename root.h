#pragma once
// create root_base with a static pointer to gc (initialized to nullptr?). 
// also in the constructor you create a collector and point the pointer to it.

// maybe not a good design we will see later.
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