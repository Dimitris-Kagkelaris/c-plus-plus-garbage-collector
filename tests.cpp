#include "doctest.h"
#include "root.h"
#include "collector.h"
#include <cmath>

struct GCFixture {
    root_base bootstrap; //calls the rootbase constructor to create a garbage collector first
    collector &gc = *root_base::get_garbage_collector();
    collector::collection_mode previous_mode;
    GCFixture() {
        previous_mode = gc.mode;
        gc.mode = collector::collection_mode::Manual;
        REQUIRE(gc.get_registry().size() == 0);
        REQUIRE(gc.get_metadata().size() == 0);
        REQUIRE(gc.get_heap_bytes() == 0);
        collector defaults;   // fresh collector: holds the defaults from collector.h
        gc.set_growth_factor(defaults.get_growth_factor());
        gc.set_next_gc(defaults.get_next_gc());
    }
    ~GCFixture() {
        gc.mark(); gc.sweep();
        CHECK(gc.get_registry().size() == 0);
        CHECK(gc.get_metadata().size() == 0);
        CHECK(gc.get_heap_bytes() == 0);
        gc.mode = previous_mode;
    }
};

TEST_SUITE_BEGIN("mark_and_sweep");

TEST_CASE_FIXTURE(GCFixture, "normal case"){
    {
        CHECK(gc.get_heap_bytes() == 0);
        root<int> a = gc.allocate<int>();
        CHECK(gc.get_heap_bytes() == sizeof(int));
        *a = 5;
        root<int> b; // this should point to null.
        root<int> c; // this should point to null in the beginnning and then point to something.
        CHECK(*a == 5);
        CHECK(b.get_ptr() == nullptr);
        CHECK(c.get_ptr() == nullptr);
        c = gc.allocate<int>();
        CHECK(gc.get_heap_bytes() == 2*sizeof(int));
        CHECK(c.get_ptr() != nullptr);

        CHECK(gc.get_metadata().size() == 2);
        gc.mark();
        CHECK(gc.isMarked(a.get_ptr()) == true);
        CHECK(gc.isMarked(c.get_ptr()) == true);
        CHECK_THROWS_AS(gc.isMarked(b.get_ptr()), std::logic_error);
        gc.sweep();     
        CHECK(gc.isMarked(a.get_ptr()) == false);
        CHECK(gc.isMarked(c.get_ptr()) == false);
        CHECK_THROWS_AS(gc.isMarked(b.get_ptr()), std::logic_error);
        CHECK(gc.get_metadata().size() == 2);
        
    }

    gc.mark();
    gc.sweep();
    CHECK(gc.get_metadata().size() == 0);
    CHECK(gc.get_heap_bytes() == 0);

}

TEST_CASE_FIXTURE(GCFixture, "mark and sweep"){
    collector &gc = *root_base::get_garbage_collector();
    // something simple
    {
        CHECK(gc.get_heap_bytes() == 0);
        root<int> a = gc.allocate<int>();
        *a = 5;
        gc.collect();
        CHECK(gc.get_heap_bytes() == sizeof(int));
        CHECK(gc.isMarked(a.get_ptr()) == false);
        CHECK(gc.get_metadata().size() == 1);
    }
    CHECK(gc.get_heap_bytes() == sizeof(int));
    gc.mark();
    CHECK(gc.get_heap_bytes() == sizeof(int));
    gc.sweep();
    CHECK(gc.get_heap_bytes() == 0);
    CHECK(gc.get_metadata().size() == 0);
    {   
        root<int> b = gc.allocate<int>();
        *b = 6;
        gc.mark();
        gc.sweep();
    }
    // root died but object is still alive
    CHECK(gc.get_registry().size() == 0);
    CHECK(gc.get_metadata().size() == 1);
    CHECK(gc.get_heap_bytes() == sizeof(int));
    gc.collect();
    CHECK(gc.get_heap_bytes() == 0);
    CHECK(gc.get_metadata().size() == 0);
}
TEST_CASE_FIXTURE(GCFixture, "deep mark and sweep (complex)"){
    // something more complex
    collector &gc = *root_base::get_garbage_collector();
    int **help_p, **help_q;

    {
        root<int *> p = gc.allocate<int*>();
        root<int *> q = gc.allocate<int*>();
        CHECK(gc.get_heap_bytes() == 2*sizeof(int*));
        *p = gc.allocate<int>();
        *q = gc.allocate<int>();
        CHECK(gc.get_heap_bytes() == 2*sizeof(int) + 2*sizeof(int*));
        help_p = p.get_ptr();
        help_q = q.get_ptr();
        **p = 5;
        **q = 4;
        CHECK(**p == 5); CHECK(**q == 4);
        gc.mark();
        CHECK(gc.isMarked(p.get_ptr()) == true);
        CHECK(gc.isMarked(q.get_ptr()) == true);
        CHECK(gc.isMarked(*p) == true);
        CHECK(gc.isMarked(*q) == true);
        CHECK(gc.get_metadata().size() == 4);
        gc.sweep();
        CHECK(gc.isMarked(p.get_ptr()) == false);
        CHECK(gc.isMarked(q.get_ptr()) == false);
        CHECK(gc.isMarked(*p) == false);
        CHECK(gc.isMarked(*q) == false);
        CHECK(gc.get_metadata().size() == 4);
    
        
        CHECK(gc.get_registry().size() == 2);
        CHECK(gc.get_heap_bytes() == 2*sizeof(int) + 2*sizeof(int*));
    }
    CHECK(gc.get_registry().size() == 0);

    gc.mark();
    CHECK(gc.get_heap_bytes() == 2*sizeof(int) + 2*sizeof(int*));
    CHECK(gc.isMarked(help_p) == false);
    CHECK(gc.isMarked(help_q) == false);
    CHECK(gc.isMarked(*help_p) == false);
    CHECK(gc.isMarked(*help_q) == false);
    CHECK(gc.get_metadata().size() == 4);
    gc.sweep();
    CHECK(gc.get_metadata().size() == 0);
    CHECK(gc.get_heap_bytes() == 0);

}

class my_obj{
    public:
        int *a;
        int aa;
        int *b;
        char *c;
        int **d;
        bool e;
        int *f;
        my_obj *other_object;
        my_obj *another_object;
        int* not_garbage_collected;
        void trace(std::vector<void *> &children){
            children.push_back(a);
            children.push_back(b);
            children.push_back(c);
            children.push_back(d);
            children.push_back(f);
            children.push_back(other_object);
            children.push_back(another_object);
            children.push_back(not_garbage_collected);
        }
};

TEST_CASE_FIXTURE(GCFixture, "object mark and sweep"){
    collector &gc = *root_base::get_garbage_collector();
    {
        my_obj* test = gc.allocate<my_obj>();
        CHECK(gc.get_heap_bytes() == sizeof(my_obj));
        CHECK(test->f == nullptr);
        CHECK(test->a == nullptr);
        CHECK(test->aa == 0);
        test->f = gc.allocate<int>();
        test->a = gc.allocate<int>(); *(test->a) = 1;
        test->b = gc.allocate<int>(); *(test->b) = 2;
        test->c = gc.allocate<char>(); *(test->c) = 'a';
        test->d = gc.allocate<int*>(); 
        test->other_object = gc.allocate<my_obj>();
        *(test->d) = gc.allocate<int>(); **(test->d) = 4;
        test->e = true;
        const size_t live_bytes = 2*sizeof(my_obj) + 4*sizeof(int) + sizeof(char) + sizeof(int*);
        CHECK(gc.get_heap_bytes() == live_bytes);
        test->not_garbage_collected = new int(10);
        CHECK(gc.get_heap_bytes() == live_bytes);   // a raw new is not the collector's
        root<my_obj> r = test;
        CHECK(*(r->f) == 0);
        CHECK(r->e == true);
        CHECK(*(r->a) == 1);
        CHECK(*(r->b) == 2);
        CHECK(*(r->c) == 'a');
        CHECK(**(r->d) == 4);
        gc.mark();
        my_obj *new_test = r.get_ptr();
        CHECK(gc.isMarked(new_test) == true);
        CHECK(gc.isMarked(new_test->a) == true);
        CHECK(gc.isMarked(new_test->b) == true);
        CHECK(gc.isMarked(new_test->c) == true);
        CHECK(gc.isMarked(new_test->d) == true);
        CHECK(gc.isMarked(*(new_test->d)) == true);
        CHECK(gc.isMarked(new_test->f) == true);
        CHECK(gc.isMarked(new_test->other_object) == true);
        gc.sweep();
        CHECK(gc.isMarked(new_test) == false);
        CHECK(gc.isMarked(new_test->a) == false);
        CHECK(gc.isMarked(new_test->b) == false);
        CHECK(gc.isMarked(new_test->c) == false);
        CHECK(gc.isMarked(new_test->d) == false);
        CHECK(gc.isMarked(*(new_test->d)) == false);
        CHECK(gc.isMarked(new_test->f) == false);
        CHECK(gc.get_metadata().size() == 8);
        CHECK(gc.get_heap_bytes() == live_bytes);   // reachable, nothing freed
        CHECK(gc.isMarked(new_test->other_object) == false);
        gc.collect();
        CHECK(gc.get_metadata().size() == 8);
        CHECK(gc.get_heap_bytes() == live_bytes);
        delete new_test->not_garbage_collected;
        new_test->not_garbage_collected = nullptr;
    }
    CHECK(gc.get_registry().size() == 0);
    gc.mark();
    CHECK(gc.get_metadata().size() == 8);
    gc.sweep();
    CHECK(gc.get_metadata().size() == 0);
    CHECK(gc.get_heap_bytes() == 0);
}

TEST_CASE_FIXTURE(GCFixture, "object mark and sweep with cycles"){
    collector &gc = *root_base::get_garbage_collector();
    {
        my_obj* object1 = gc.allocate<my_obj>();
        my_obj* object1_point_5 = gc.allocate<my_obj>();
        my_obj* object2 = gc.allocate<my_obj>();
        my_obj* object3 = gc.allocate<my_obj>();
        my_obj* object4 = gc.allocate<my_obj>();
        my_obj* object5 = gc.allocate<my_obj>();
        CHECK(gc.get_heap_bytes() == 6*sizeof(my_obj));
        object1->other_object = object1_point_5;
        object1_point_5->other_object = object2;
        object2->other_object = object1;
        object3->other_object = object4;
        object4->other_object = object3;
        object5->other_object = object1;
        root<my_obj> r1 = object1;
        {
            root<my_obj> r2 = object3;
            CHECK(gc.get_metadata().size() == 6);
            gc.collect();
            CHECK(gc.get_metadata().size() == 5);
            CHECK(gc.get_heap_bytes() == 5*sizeof(my_obj));   // only object5 dropped
        }
        gc.collect();
        CHECK(gc.get_metadata().size() == 3);
        CHECK(gc.get_heap_bytes() == 3*sizeof(my_obj));       // the 3/4 cycle is gone
    }
    gc.collect();
    CHECK(gc.get_metadata().size() == 0);
    CHECK(gc.get_heap_bytes() == 0);                      // and object1's cycle too
}

TEST_SUITE_END();

TEST_SUITE_BEGIN("roots");

TEST_CASE_FIXTURE(GCFixture, "root1"){
    root<int> a;
    collector &gc = *root_base::get_garbage_collector();
    CHECK(gc.get_registry().size() == 1);
    {   
        root<int> rr = new int(3);
        CHECK((*rr == *(rr.get_ptr())));
        CHECK(gc.get_registry().size() == 2);
        CHECK(gc.get_heap_bytes() == 0);   // raw new, not tracked
        delete rr.get_ptr();
    }
    CHECK(gc.get_registry().size() == 1);
    struct t {
        int a;
        char b;
        bool c;
        t(int a, char b, bool c): a(a), b(b), c(c) {}
    };
    root<struct t> r = new struct t(1, 'a', true);
    CHECK(((*r).a == 1));
    CHECK((r->b == 'a'));
    CHECK((r->c == true));
    CHECK(gc.get_registry().size() == 2);
    CHECK(gc.get_heap_bytes() == 0);
    delete r.get_ptr();
}

TEST_CASE_FIXTURE(GCFixture, "root2"){
    root<int> a = new int [10];
    collector &gc = *root_base::get_garbage_collector();
    for(int i = 0; i < 10; ++i){
        a[i] = i+1;//*(a+i)
    }
    for(int i = 0; i < 10; ++i){
        CHECK(a[i] == i+1);
    }
    CHECK(gc.get_registry().size() == 1);
    std::vector<void**> reg = gc.get_registry();
    CHECK(*a == *(a.get_ptr()));

    CHECK(*reg[0] == a.get_ptr());
    CHECK(*static_cast<int*>(*reg[0]) == *a);
    CHECK(gc.get_heap_bytes() == 0);   // raw new[], not tracked
    


    delete[] a.get_ptr();
    a = new int;
    *a = 3;
    CHECK(*a == 3);
    CHECK(gc.get_registry().size() == 1);
    CHECK(gc.get_heap_bytes() == 0);
    delete a.get_ptr();
}

TEST_CASE_FIXTURE(GCFixture, "root3"){
    collector &gc = *root_base::get_garbage_collector();
    {
        root<int> a = new int(6);
        root<int> b = new int(2);
        void *bb_copy;
        {
            root<int> bb = b;
            
            CHECK(bb.get_ptr() == b.get_ptr());
            CHECK(gc.get_registry().size() == 3);
            
            std::vector<void**> reg = gc.get_registry();
            CHECK(*reg[2] == *reg[1]);
            CHECK(*reg[0] != *reg[1]);
            bb_copy = bb.get_ptr();
            CHECK(gc.get_heap_bytes() == 0);   // copying a root allocates nothing
            
        }
        delete a.get_ptr();
        a = nullptr;
        CHECK(gc.get_registry().size() == 2);
        CHECK(b.get_ptr() == bb_copy);
        std::vector<void**> reg = gc.get_registry();
        CHECK(*reg[0] == nullptr);
        CHECK(*reg[1] == bb_copy);
        CHECK(gc.get_heap_bytes() == 0);
        
        delete b.get_ptr();
    }
    root<int> cc = new int(4);
    CHECK(gc.get_registry().size() == 1);
    CHECK(gc.get_heap_bytes() == 0);
    delete cc.get_ptr();
}

TEST_CASE_FIXTURE(GCFixture, "mark ignores null and non-GC children") {
    root<my_obj> r = gc.allocate<my_obj>();
    r->a = gc.allocate<int>();
    CHECK(gc.get_heap_bytes() == sizeof(my_obj) + sizeof(int));
    r->not_garbage_collected = new int(10);
    CHECK(gc.get_heap_bytes() == sizeof(my_obj) + sizeof(int));   // untracked child

    CHECK_NOTHROW(gc.mark());
    CHECK(gc.isMarked(r->a) == true);

    gc.sweep();
    CHECK(gc.get_heap_bytes() == sizeof(my_obj) + sizeof(int));   // both still reachable
    CHECK(*(r->not_garbage_collected) == 10);
    delete r->not_garbage_collected;
}
TEST_SUITE_END();


TEST_SUITE_BEGIN("edge_cases");

TEST_CASE_FIXTURE(GCFixture, "deep chain does not overflow the stack") {
    constexpr int kDepth = 1000000;

    {
        my_obj *head = gc.allocate<my_obj>();
        root<my_obj> r = head;

        my_obj *tail = head;
        for (int i = 1; i < kDepth; ++i) {
            tail->other_object = gc.allocate<my_obj>();
            tail = tail->other_object;
        }

        CHECK(gc.get_metadata().size() == kDepth);
        CHECK(gc.get_heap_bytes() == static_cast<size_t>(kDepth) * sizeof(my_obj));

        gc.mark();                       // stack overflow here if mark() recurses

        CHECK(gc.isMarked(head) == true);
        CHECK(gc.isMarked(tail) == true);   // the far end was reached

        gc.sweep();
        CHECK(gc.get_metadata().size() == kDepth);   // all still reachable
        CHECK(gc.get_heap_bytes() == static_cast<size_t>(kDepth) * sizeof(my_obj));
    }

    gc.collect();
    CHECK(gc.get_metadata().size() == 0);            // root gone, whole chain freed
    CHECK(gc.get_heap_bytes() == 0);                 // and every byte accounted for
}

TEST_CASE_FIXTURE(GCFixture, "shared child is marked once and freed once") {
    {
        root<my_obj> r = gc.allocate<my_obj>();

        my_obj *left  = gc.allocate<my_obj>();
        my_obj *right = gc.allocate<my_obj>();
        int *shared = gc.allocate<int>();
        *shared = 7;

        r->other_object = left;
        left->other_object = right;   // my_obj has only one my_obj* member, so chain them
        left->a  = shared;
        right->a = shared;            // reachable via two distinct paths

        CHECK(gc.get_metadata().size() == 4);
        const size_t live_bytes = 3*sizeof(my_obj) + sizeof(int);
        CHECK(gc.get_heap_bytes() == live_bytes);

        gc.mark();
        CHECK(gc.isMarked(shared) == true);
        CHECK(gc.isMarked(left)   == true);
        CHECK(gc.isMarked(right)  == true);

        gc.sweep();
        CHECK(gc.get_metadata().size() == 4);   // nothing freed, all reachable
        CHECK(gc.get_heap_bytes() == live_bytes);
        CHECK(*shared == 7);                    // and the shared node is intact
    }

    gc.collect();
    CHECK(gc.get_metadata().size() == 0);       // freed exactly once, no double free
    CHECK(gc.get_heap_bytes() == 0);            // a double subtract would underflow
}


TEST_CASE_FIXTURE(GCFixture, "shared child survives while any path remains") {
    {
        root<my_obj> r = gc.allocate<my_obj>();
        my_obj *left = gc.allocate<my_obj>();
        int *shared = gc.allocate<int>();
        *shared = 7;

        r->other_object = left;
        r->a = shared;
        left->a = shared;

        gc.collect();
        CHECK(gc.get_metadata().size() == 3);
        CHECK(gc.get_heap_bytes() == 2*sizeof(my_obj) + sizeof(int));

        left->a = nullptr;              // one path gone, the other remains
        gc.collect();
        CHECK(gc.get_metadata().size() == 3);
        CHECK(gc.get_heap_bytes() == 2*sizeof(my_obj) + sizeof(int));   // nothing freed
        CHECK(*shared == 7);

        r->a = nullptr;                 // last path gone
        gc.collect();
        CHECK(gc.get_metadata().size() == 2);
        CHECK(gc.get_heap_bytes() == 2*sizeof(my_obj));                 // the int, exactly
    }

    gc.collect();
    CHECK(gc.get_metadata().size() == 0);
    CHECK(gc.get_heap_bytes() == 0);
}

TEST_CASE_FIXTURE(GCFixture, "self-referencing object") {
    {
        root<my_obj> r = gc.allocate<my_obj>();
        r->other_object = r.get_ptr();      // points at itself

        CHECK(gc.get_metadata().size() == 1);
        CHECK(gc.get_heap_bytes() == sizeof(my_obj));

        gc.mark();                          // must terminate
        CHECK(gc.isMarked(r.get_ptr()) == true);

        gc.sweep();
        CHECK(gc.get_metadata().size() == 1);
        CHECK(gc.get_heap_bytes() == sizeof(my_obj));
    }

    gc.collect();
    CHECK(gc.get_metadata().size() == 0);   // unreachable self-loop is collected
    CHECK(gc.get_heap_bytes() == 0);
}

TEST_CASE_FIXTURE(GCFixture, "self-loop inside a larger cycle") {
    {
        root<my_obj> r = gc.allocate<my_obj>();
        my_obj *a = gc.allocate<my_obj>();
        my_obj *b = gc.allocate<my_obj>();

        r->other_object = a;
        a->other_object = b;
        b->other_object = a;        // 2-cycle
        a->another_object = a;      // and a self-loop on one of its members

        gc.mark();
        CHECK(gc.isMarked(a) == true);
        CHECK(gc.isMarked(b) == true);
        gc.sweep();
        CHECK(gc.get_metadata().size() == 3);
        CHECK(gc.get_heap_bytes() == 3*sizeof(my_obj));
    }

    gc.collect();
    CHECK(gc.get_metadata().size() == 0);
    CHECK(gc.get_heap_bytes() == 0);
}

TEST_CASE_FIXTURE(GCFixture, "marking twice is idempotent") {
    root<my_obj> r = gc.allocate<my_obj>();
    r->a = gc.allocate<int>();
    *(r->a) = 3;
    CHECK(gc.get_heap_bytes() == sizeof(my_obj) + sizeof(int));

    gc.mark();
    CHECK(gc.isMarked(r.get_ptr()) == true);
    CHECK(gc.isMarked(r->a) == true);

    CHECK_NOTHROW(gc.mark());               // second pass over already-marked objects
    CHECK(gc.isMarked(r.get_ptr()) == true);
    CHECK(gc.isMarked(r->a) == true);
    CHECK(gc.get_metadata().size() == 2);   // nothing added or lost
    CHECK(gc.get_heap_bytes() == sizeof(my_obj) + sizeof(int));   // mark never frees

    gc.sweep();
    CHECK(gc.get_metadata().size() == 2);
    CHECK(gc.get_heap_bytes() == sizeof(my_obj) + sizeof(int));
}

TEST_CASE_FIXTURE(GCFixture, "collecting an empty heap is a no-op") {
    CHECK(gc.get_metadata().size() == 0);
    CHECK(gc.get_heap_bytes() == 0);

    CHECK_NOTHROW(gc.collect());
    CHECK(gc.get_metadata().size() == 0);
    CHECK(gc.get_heap_bytes() == 0);

    CHECK_NOTHROW(gc.collect());            // and again
    CHECK(gc.get_metadata().size() == 0);
    CHECK(gc.get_heap_bytes() == 0);        // no unsigned underflow
}


TEST_CASE_FIXTURE(GCFixture, "collecting with no roots frees everything") {
    gc.allocate<my_obj>();
    gc.allocate<int>();
    gc.allocate<char>();
    CHECK(gc.get_metadata().size() == 3);
    CHECK(gc.get_heap_bytes() == sizeof(my_obj) + sizeof(int) + sizeof(char));

    gc.collect();
    CHECK(gc.get_metadata().size() == 0);
    CHECK(gc.get_heap_bytes() == 0);

    CHECK_NOTHROW(gc.collect());            // sweep over a now-empty heap
    CHECK(gc.get_metadata().size() == 0);
    CHECK(gc.get_heap_bytes() == 0);
}

struct counted {
    static int destroyed;
    ~counted() { ++destroyed; }
    void trace(std::vector<void *> &){}
};
int counted::destroyed = 0;

TEST_CASE_FIXTURE(GCFixture, "sweep runs destructors") {
    counted::destroyed = 0;
    gc.allocate<counted>();      // no root, immediately garbage
    CHECK(gc.get_heap_bytes() == sizeof(counted));
    gc.collect();
    CHECK(counted::destroyed == 1);
    CHECK(gc.get_heap_bytes() == 0);
    counted::destroyed = 0;
}

TEST_SUITE_END();

TEST_SUITE_BEGIN("normal mode");

TEST_CASE_FIXTURE(GCFixture, "collects on the first allocation after reaching the threshold") {
    gc.mode = collector::collection_mode::Normal;
    const size_t threshold = gc.get_next_gc();

    gc.allocate<char>(threshold - 1);   // garbage, no root
    gc.allocate<char>();                // check sees threshold - 1 < threshold: no collection
    CHECK(gc.get_metadata().size() == 2);
    CHECK(gc.get_heap_bytes() == threshold);
    CHECK(gc.get_next_gc() == threshold);   // untouched

    gc.allocate<char>();                // check sees threshold >= threshold: collects, then allocates
    CHECK(gc.get_metadata().size() == 1);          // both garbage chars freed, only the new one left
    CHECK(gc.get_heap_bytes() == sizeof(char));
    CHECK(gc.get_next_gc() == MB);                 // nothing survived: falls back to the MB floor
}

TEST_CASE_FIXTURE(GCFixture, "automatic collection keeps rooted objects") {
    gc.mode = collector::collection_mode::Normal;
    const size_t threshold = gc.get_next_gc();

    root<my_obj> r = gc.allocate<my_obj>();
    r->a = gc.allocate<int>(); *(r->a) = 7;
    r->other_object = gc.allocate<my_obj>();
    const size_t live_bytes = 2*sizeof(my_obj) + sizeof(int);

    gc.allocate<char>(threshold - live_bytes);   // garbage fills the heap up to the threshold
    CHECK(gc.get_heap_bytes() == threshold);
    gc.allocate<char>();                         // triggers the collection

    CHECK(gc.get_metadata().size() == 4);        // the 3 rooted objects + the new char
    CHECK(gc.get_heap_bytes() == live_bytes + sizeof(char));
    CHECK(*(r->a) == 7);                         // survivor still usable
    CHECK(gc.get_next_gc() == MB);               // live heap is tiny: still the floor
}

TEST_CASE_FIXTURE(GCFixture, "next_gc grows by growth_factor when a lot survives") {
    gc.mode = collector::collection_mode::Normal;
    gc.set_growth_factor(3);            // non-default, so the test proves the factor is actually used
    const size_t threshold = gc.get_next_gc();

    root<char> big = gc.allocate<char>(threshold - 1);   // rooted: survives the collection
    gc.allocate<char>();                                  // garbage, heap reaches the threshold
    gc.allocate<char>();                                  // triggers the collection

    const size_t survived = threshold - 1;
    CHECK(gc.get_metadata().size() == 2);                 // big + the new char
    CHECK(gc.get_heap_bytes() == survived + sizeof(char));
    CHECK(gc.get_next_gc() == static_cast<size_t>(survived * gc.get_growth_factor()));
}

TEST_CASE_FIXTURE(GCFixture, "set_growth_factor rejects factors outside (1, 100) and NaN") {
    const double factor = gc.get_growth_factor();
    CHECK_THROWS_AS(gc.set_growth_factor(1.0), std::invalid_argument);   // lower boundary
    CHECK_THROWS_AS(gc.set_growth_factor(0.5), std::invalid_argument);
    CHECK_THROWS_AS(gc.set_growth_factor(-2), std::invalid_argument);
    CHECK_THROWS_AS(gc.set_growth_factor(100.0), std::invalid_argument);   // upper boundary, exclusive
    CHECK_THROWS_AS(gc.set_growth_factor(INFINITY), std::invalid_argument);
    CHECK_THROWS_AS(gc.set_growth_factor(std::nan("")), std::invalid_argument);
    CHECK(gc.get_growth_factor() == factor);    // rejected values leave it unchanged

    gc.set_growth_factor(1.5);
    CHECK(gc.get_growth_factor() == 1.5);
    gc.set_growth_factor(std::nextafter(100.0, 0.0));   // largest double below 100 is accepted
    CHECK(gc.get_growth_factor() == std::nextafter(100.0, 0.0));
}

TEST_CASE_FIXTURE(GCFixture, "set_next_gc rejects values <= heap_bytes and clamps to MB") {
    gc.allocate<char>(16);                      // heap_bytes = 16; Manual mode, so it stays
    CHECK_THROWS_AS(gc.set_next_gc(16), std::invalid_argument);   // equal to heap_bytes
    CHECK_THROWS_AS(gc.set_next_gc(0), std::invalid_argument);
    CHECK(gc.get_next_gc() == MB);              // rejected values leave it unchanged

    gc.set_next_gc(17);                         // valid, but below the floor
    CHECK(gc.get_next_gc() == MB);
    gc.set_next_gc(3 * MB);
    CHECK(gc.get_next_gc() == 3 * MB);
}

TEST_CASE_FIXTURE(GCFixture, "manual mode ignores the threshold") {
    gc.mode = collector::collection_mode::Manual;   // fixture default, set again to make the test explicit
    const size_t threshold = gc.get_next_gc();

    gc.allocate<char>(threshold);           // garbage, heap reaches the threshold
    gc.allocate<char>(threshold);           // Normal would collect here
    gc.allocate<char>();                    // and here
    CHECK(gc.get_metadata().size() == 3);
    CHECK(gc.get_heap_bytes() == 2 * threshold + sizeof(char));
    CHECK(gc.get_next_gc() == threshold);   // never updated

    gc.collect();                           // an explicit collect still frees everything
    CHECK(gc.get_metadata().size() == 0);
    CHECK(gc.get_heap_bytes() == 0);
    CHECK(gc.get_next_gc() == threshold);   // collect() itself doesn't touch next_gc
}

TEST_SUITE_END();

TEST_SUITE_BEGIN("stress mode");

TEST_CASE_FIXTURE(GCFixture, "unrooted object is freed by the next allocation") {
    gc.mode = collector::collection_mode::Stress;
    counted::destroyed = 0;

    gc.allocate<counted>();                 // no root
    CHECK(gc.get_metadata().size() == 1);   // its own allocation never collects it
    CHECK(counted::destroyed == 0);

    gc.allocate<int>();                     // collects first: the counted is gone
    CHECK(counted::destroyed == 1);
    CHECK(gc.get_metadata().size() == 1);   // only the int
    CHECK(gc.get_heap_bytes() == sizeof(int));
    CHECK(gc.get_next_gc() == MB);          // Stress doesn't touch the threshold
    counted::destroyed = 0;
}

TEST_CASE_FIXTURE(GCFixture, "rooted graph with a cycle survives every allocation") {
    gc.mode = collector::collection_mode::Stress;

    root<my_obj> r = gc.allocate<my_obj>();
    r->other_object = gc.allocate<my_obj>();        // r is rooted, so this call's collection keeps it
    r->other_object->other_object = r.get_ptr();    // cycle back to the rooted object
    r->a = gc.allocate<int>(); *(r->a) = 42;
    const size_t live_bytes = 2*sizeof(my_obj) + sizeof(int);

    for (int i = 0; i < 100; ++i) {
        gc.allocate<char>();                        // each call frees the previous char
        CHECK(gc.get_metadata().size() == 4);       // the 3 live objects + this char
        CHECK(gc.get_heap_bytes() == live_bytes + sizeof(char));
    }
    CHECK(*(r->a) == 42);
    CHECK(r->other_object->other_object == r.get_ptr());   // cycle intact
}

TEST_CASE_FIXTURE(GCFixture, "reassigned root's old target is freed on the following allocation") {
    gc.mode = collector::collection_mode::Stress;
    counted::destroyed = 0;

    root<counted> r = gc.allocate<counted>();
    counted *first = r.get_ptr();
    r = gc.allocate<counted>();             // collects before r moves: first is still rooted
    CHECK(counted::destroyed == 0);
    CHECK(gc.get_metadata().size() == 2);
    CHECK(r.get_ptr() != first);

    gc.allocate<int>();                     // now first is unreachable
    CHECK(counted::destroyed == 1);
    CHECK_NOTHROW(gc.isMarked(r.get_ptr()));   // the survivor is the second one, so first was freed
    CHECK(gc.get_metadata().size() == 2);   // the second counted + the int
    CHECK(gc.get_heap_bytes() == sizeof(counted) + sizeof(int));
    counted::destroyed = 0;
}

TEST_SUITE_END();