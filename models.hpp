constexpr size_t MB = 1024*1024;

enum class collection_mode {
    Normal,     // collect when heap_bytes reaches next_gc
    Stress,     // collect after every allocation
    Manual      // collect only on explicit collect() calls
};

struct allocation {
    bool marked;
    std::function<std::vector<void *>(void)> trace;
    std::function<size_t(void)> deallocate;
};