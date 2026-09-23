#include "doctest.h"
#include "root.h"
#include "collector.h"

struct GCFixture {
    root_base bootstrap; //calls the rootbase constructor to create a garbage collector first
    collector &gc = *root_base::get_garbage_collector();
    GCFixture() {
        REQUIRE(gc.get_registry().size() == 0);
        REQUIRE(gc.get_metadata().size() == 0);
    }
    ~GCFixture() {
        gc.mark(); gc.sweep();
        CHECK(gc.get_registry().size() == 0);
        CHECK(gc.get_metadata().size() == 0);
    }

};

TEST_SUITE_BEGIN("mark_and_sweep");

TEST_CASE_FIXTURE(GCFixture, "normal case"){
    {
        root<int> a = gc.allocate<int>();
        *a = 5;
        root<int> b; // this should point to null.
        root<int> c; // this should point to null in the beginnning and then point to something.
        CHECK(*a == 5);
        CHECK(b.get_ptr() == nullptr);
        CHECK(c.get_ptr() == nullptr);
        c = gc.allocate<int>();
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

}

TEST_CASE_FIXTURE(GCFixture, "mark and sweep"){
    collector &gc = *root_base::get_garbage_collector();
    // something simple
    {
        root<int> a = gc.allocate<int>();
        *a = 5;
        gc.collect();
        CHECK(gc.isMarked(a.get_ptr()) == false);
        CHECK(gc.get_metadata().size() == 1);
    }

    gc.mark();
    gc.sweep();
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
    gc.collect();
    CHECK(gc.get_metadata().size() == 0);
}
TEST_CASE_FIXTURE(GCFixture, "deep mark and sweep (complex)"){
    // something more complex
    collector &gc = *root_base::get_garbage_collector();
    int **help_p, **help_q;

    {
        root<int *> p = gc.allocate<int*>();
        root<int *> q = gc.allocate<int*>();
        *p = gc.allocate<int>();
        *q = gc.allocate<int>();
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
    }
    CHECK(gc.get_registry().size() == 0);

    gc.mark();
    CHECK(gc.isMarked(help_p) == false);
    CHECK(gc.isMarked(help_q) == false);
    CHECK(gc.isMarked(*help_p) == false);
    CHECK(gc.isMarked(*help_q) == false);
    CHECK(gc.get_metadata().size() == 4);
    gc.sweep();
    CHECK(gc.get_metadata().size() == 0);

    // in what order should the marking and sweeping happen?
    // marking in the way the registry is. sweeping is unordered.

    // try and delete manually the allocations


}

class t2{
    public:
        int *a;
        int aa;
        int *b;
        char *c;
        int **d;
        bool e;
        int *f;
        t2 *other_object;
        t2 *another_object;
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
        t2* test = gc.allocate<t2>();
        CHECK(test->f == nullptr);
        CHECK(test->a == nullptr);
        CHECK(test->aa == 0);
        test->f = gc.allocate<int>();
        test->a = gc.allocate<int>(); *(test->a) = 1;
        test->b = gc.allocate<int>(); *(test->b) = 2;
        test->c = gc.allocate<char>(); *(test->c) = 'a';
        test->d = gc.allocate<int*>(); 
        test->other_object = gc.allocate<t2>();
        *(test->d) = gc.allocate<int>(); **(test->d) = 4;
        test->e = true;
        test->not_garbage_collected = new int(10);
        root<t2> r = test;
        CHECK(*(r->f) == 0);
        CHECK(r->e == true);
        CHECK(*(r->a) == 1);
        CHECK(*(r->b) == 2);
        CHECK(*(r->c) == 'a');
        CHECK(**(r->d) == 4);
        gc.mark();
        t2 *new_test = r.get_ptr();
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
        CHECK(gc.isMarked(new_test->other_object) == false);
        gc.collect();
        CHECK(gc.get_metadata().size() == 8);
        delete new_test->not_garbage_collected;
        new_test->not_garbage_collected = nullptr;
    }
    CHECK(gc.get_registry().size() == 0);
    gc.mark();
    CHECK(gc.get_metadata().size() == 8);
    gc.sweep();
    CHECK(gc.get_metadata().size() == 0);
}

TEST_CASE_FIXTURE(GCFixture, "object mark and sweep with cycles"){
    collector &gc = *root_base::get_garbage_collector();
    {
        t2* object1 = gc.allocate<t2>();
        t2* object1_point_5 = gc.allocate<t2>();
        t2* object2 = gc.allocate<t2>();
        t2* object3 = gc.allocate<t2>();
        t2* object4 = gc.allocate<t2>();
        t2* object5 = gc.allocate<t2>();
        object1->other_object = object1_point_5;
        object1_point_5->other_object = object2;
        object2->other_object = object1;
        object3->other_object = object4;
        object4->other_object = object3;
        object5->other_object = object1;
        root<t2> r1 = object1;
        {
            root<t2> r2 = object3;
            CHECK(gc.get_metadata().size() == 6);
            gc.collect();
            CHECK(gc.get_metadata().size() == 5);
        }
        gc.collect();
        CHECK(gc.get_metadata().size() == 3);
    }
    gc.collect();
    CHECK(gc.get_metadata().size() == 0);
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
    


    delete[] a.get_ptr();
    a = new int;
    *a = 3;
    CHECK(*a == 3);
    CHECK(gc.get_registry().size() == 1);
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
            
        }
        delete a.get_ptr();
        a = nullptr;
        CHECK(gc.get_registry().size() == 2);
        CHECK(b.get_ptr() == bb_copy);
        std::vector<void**> reg = gc.get_registry();
        CHECK(*reg[0] == nullptr);
        CHECK(*reg[1] == bb_copy);
        
        delete b.get_ptr();
    }
    root<int> cc = new int(4);
    CHECK(gc.get_registry().size() == 1);
    delete cc.get_ptr();
}

TEST_CASE_FIXTURE(GCFixture, "mark ignores null and non-GC children") {
    root<t2> r = gc.allocate<t2>();
    r->a = gc.allocate<int>();
    r->not_garbage_collected = new int(10);

    CHECK_NOTHROW(gc.mark());
    CHECK(gc.isMarked(r->a) == true);

    gc.sweep();
    CHECK(*(r->not_garbage_collected) == 10);
    delete r->not_garbage_collected;
}
TEST_SUITE_END();


// new tests:
TEST_SUITE_BEGIN("edge_cases");

TEST_CASE_FIXTURE(GCFixture, "deep chain does not overflow the stack") {
    constexpr int kDepth = 1000000;

    {
        t2 *head = gc.allocate<t2>();
        root<t2> r = head;

        t2 *tail = head;
        for (int i = 1; i < kDepth; ++i) {
            tail->other_object = gc.allocate<t2>();
            tail = tail->other_object;
        }

        CHECK(gc.get_metadata().size() == kDepth);

        gc.mark();                       // stack overflow here if mark() recurses

        CHECK(gc.isMarked(head) == true);
        CHECK(gc.isMarked(tail) == true);   // the far end was reached

        gc.sweep();
        CHECK(gc.get_metadata().size() == kDepth);   // all still reachable
    }

    gc.collect();
    CHECK(gc.get_metadata().size() == 0);            // root gone, whole chain freed
}

TEST_CASE_FIXTURE(GCFixture, "shared child is marked once and freed once") {
    {
        root<t2> r = gc.allocate<t2>();

        t2 *left  = gc.allocate<t2>();
        t2 *right = gc.allocate<t2>();
        int *shared = gc.allocate<int>();
        *shared = 7;

        r->other_object = left;
        left->other_object = right;   // t2 has only one t2* member, so chain them
        left->a  = shared;
        right->a = shared;            // reachable via two distinct paths

        CHECK(gc.get_metadata().size() == 4);

        gc.mark();
        CHECK(gc.isMarked(shared) == true);
        CHECK(gc.isMarked(left)   == true);
        CHECK(gc.isMarked(right)  == true);

        gc.sweep();
        CHECK(gc.get_metadata().size() == 4);   // nothing freed, all reachable
        CHECK(*shared == 7);                    // and the shared node is intact
    }

    gc.collect();
    CHECK(gc.get_metadata().size() == 0);       // freed exactly once, no double free
}


TEST_CASE_FIXTURE(GCFixture, "shared child survives while any path remains") {
    {
        root<t2> r = gc.allocate<t2>();
        t2 *left = gc.allocate<t2>();
        int *shared = gc.allocate<int>();
        *shared = 7;

        r->other_object = left;
        r->a = shared;
        left->a = shared;

        gc.collect();
        CHECK(gc.get_metadata().size() == 3);

        left->a = nullptr;              // one path gone, the other remains
        gc.collect();
        CHECK(gc.get_metadata().size() == 3);
        CHECK(*shared == 7);

        r->a = nullptr;                 // last path gone
        gc.collect();
        CHECK(gc.get_metadata().size() == 2);
    }

    gc.collect();
    CHECK(gc.get_metadata().size() == 0);
}

TEST_CASE_FIXTURE(GCFixture, "self-referencing object") {
    {
        root<t2> r = gc.allocate<t2>();
        r->other_object = r.get_ptr();      // points at itself

        CHECK(gc.get_metadata().size() == 1);

        gc.mark();                          // must terminate
        CHECK(gc.isMarked(r.get_ptr()) == true);

        gc.sweep();
        CHECK(gc.get_metadata().size() == 1);
    }

    gc.collect();
    CHECK(gc.get_metadata().size() == 0);   // unreachable self-loop is collected
}

TEST_CASE_FIXTURE(GCFixture, "self-loop inside a larger cycle") {
    {
        root<t2> r = gc.allocate<t2>();
        t2 *a = gc.allocate<t2>();
        t2 *b = gc.allocate<t2>();

        r->other_object = a;
        a->other_object = b;
        b->other_object = a;        // 2-cycle
        a->another_object = a;      // and a self-loop on one of its members

        gc.mark();
        CHECK(gc.isMarked(a) == true);
        CHECK(gc.isMarked(b) == true);
        gc.sweep();
        CHECK(gc.get_metadata().size() == 3);
    }

    gc.collect();
    CHECK(gc.get_metadata().size() == 0);
}

TEST_CASE_FIXTURE(GCFixture, "marking twice is idempotent") {
    root<t2> r = gc.allocate<t2>();
    r->a = gc.allocate<int>();
    *(r->a) = 3;

    gc.mark();
    CHECK(gc.isMarked(r.get_ptr()) == true);
    CHECK(gc.isMarked(r->a) == true);

    CHECK_NOTHROW(gc.mark());               // second pass over already-marked objects
    CHECK(gc.isMarked(r.get_ptr()) == true);
    CHECK(gc.isMarked(r->a) == true);
    CHECK(gc.get_metadata().size() == 2);   // nothing added or lost

    gc.sweep();
    CHECK(gc.get_metadata().size() == 2);
}

TEST_CASE_FIXTURE(GCFixture, "collecting an empty heap is a no-op") {
    CHECK(gc.get_metadata().size() == 0);

    CHECK_NOTHROW(gc.collect());
    CHECK(gc.get_metadata().size() == 0);

    CHECK_NOTHROW(gc.collect());            // and again
    CHECK(gc.get_metadata().size() == 0);
}


TEST_CASE_FIXTURE(GCFixture, "collecting with no roots frees everything") {
    gc.allocate<t2>();
    gc.allocate<int>();
    gc.allocate<char>();
    CHECK(gc.get_metadata().size() == 3);

    gc.collect();
    CHECK(gc.get_metadata().size() == 0);

    CHECK_NOTHROW(gc.collect());            // sweep over a now-empty heap
    CHECK(gc.get_metadata().size() == 0);
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
    gc.collect();
    CHECK(counted::destroyed == 1);
    counted::destroyed = 0;
}

TEST_SUITE_END();