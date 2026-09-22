#include "doctest.h"
#include "root.h"
#include "collector.h"

void ms(){
    collector &gc = *root_base::get_garbage_collector();
    gc.mark();
    gc.sweep();   
}

TEST_CASE("normal case"){
    root<int> dummy;
    collector &gc = *root_base::get_garbage_collector();
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

TEST_CASE("mark and sweep"){
    collector &gc = *root_base::get_garbage_collector();
    // something simple
    {
        root<int> a = gc.allocate<int>();
        *a = 5;
        ms();
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
    ms();
    CHECK(gc.get_metadata().size() == 0);
}
TEST_CASE("deep mark and sweep (complex)"){
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
        int* not_garbage_collected;
        void trace(std::vector<void *> &children){
            children.push_back(a);
            children.push_back(b);
            children.push_back(c);
            children.push_back(d);
            children.push_back(f);
            children.push_back(other_object);
            children.push_back(not_garbage_collected);
        }
};

TEST_CASE("object mark and sweep"){
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
        ms();
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

TEST_CASE("object mark and sweep with cycles"){
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
            ms();
            CHECK(gc.get_metadata().size() == 5);
        }
        ms();
        CHECK(gc.get_metadata().size() == 3);
    }
    ms();
    CHECK(gc.get_metadata().size() == 0);
}

TEST_CASE("root1"){
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

TEST_CASE("root2"){
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

TEST_CASE("root3"){
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
        
    
    }
    root<int> cc = new int(4);
    CHECK(gc.get_registry().size() == 1);
    delete cc.get_ptr();
}

// do ASAN options detect leaks
