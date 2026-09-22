#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "root.h"
#include "collector.h"

void ms(){
    collector &gc = *root_base::get_garbage_collector();
    gc.mark();
    gc.sweep();
}
void test5(){
    collector &gc = *root_base::get_garbage_collector();
    {
    root<int> a = gc.allocate<int>();
    *a = 5;
    root<int> b; // this should point to null.
    root<int> c; // this should point to null in the beginnning and then point to something.
    c = gc.allocate<int>();
    gc.print_registry();

    ms();
    
}

}
void test4(){
    collector &gc = *root_base::get_garbage_collector();
    // something simple
    {
    root<int> a = gc.allocate<int>();
    *a = 5;
    ms();

}

    gc.mark();
    gc.sweep();
    cout << "Second test" << endl;
    {   
        root<int> b = gc.allocate<int>();
        *b = 6;
        gc.mark();
        gc.sweep();
    }


     cout << "New test"<< endl;

    {root<int *> p = gc.allocate<int*>();
        root<int *> q = gc.allocate<int*>();
        *p = gc.allocate<int>();
        *q = gc.allocate<int>();
        **p = 5;
        **q = 4;
        ms();
        
        gc.print_registry();
        
    }
    
    ms();

    // in what order should the marking and sweeping happen?
    // marking in the way the registry is. sweeping is unordered.

}

class t2{
private:
    int *a;
    int *b;
    char *c;
    int **d;
    bool e;
public:
    int *ab;
    void trace(std::vector<void *> &children){
        children.push_back(a);
        children.push_back(b);
        children.push_back(c);
        children.push_back(d);
        children.push_back(ab);
    }
};

void test1(){
    collector &gc = *root_base::get_garbage_collector();
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
    

    for(auto &[_, b]: gc.get_metadata()){
        b.print_allocation();
        cout << endl;
    }
    for(auto &[_, b]: gc.get_metadata()){
        std::vector<void *> ch = b.trace();
        cout << "Number of pointers following: " << ch.size() << endl;
        for(size_t i = 0; i < ch.size(); ++i){
            cout << *(int *)(ch[i]) << endl;
        }
        cout << endl;
    }
}


void test2(){
    collector &gc = *root_base::get_garbage_collector();;
    t2 *test = gc.allocate<t2>();
    test->ab = new int(3);
    cout << *(test->ab) << endl;
    
    
    for(auto &[_, b]: gc.get_metadata()){
        std::vector<void *> ch = b.trace();
        cout << "Number of pointers following: " << ch.size() << endl;
        for(size_t i = 0; i < ch.size(); ++i){
            cout << ch[i] << endl;
            // cout << *(int *)(ch[i]) << endl;
        }
        cout << endl;
    }
}

void test3(){
    collector &gc = *root_base::get_garbage_collector();
    root<int *> p = gc.allocate<int*>();
    root<int *> q = gc.allocate<int*>();
    *p = gc.allocate<int>();
    *q = gc.allocate<int>();
    **p = 5;
    **q = 4;
    
    cout << " pointer values: "<< endl;
    // cout << p << ' ' << *p << ' ';
    cout << **p << endl;
    // cout << q << ' ' << *q << ' '; 
    cout << **q << endl;
    cout << endl;
    

    for(auto &[_, b]: gc.get_metadata()){
        b.print_allocation();
        cout << endl;
    }
    for(auto &[_, b]: gc.get_metadata()){
        std::vector<void *> ch = b.trace();
        cout << "Number of pointers following: " << ch.size() << endl;
        for(size_t i = 0; i < ch.size(); ++i){
            cout << *(int *)(ch[i]) << endl;
        }
        cout << endl;
    }
    // cout << "One" << endl;
    // delete *p;
    // cout << "One" << endl;
    // delete *q;
    // cout << "One" << endl;
    // delete p.get_ptr();
    // cout << "One" << endl;
    // delete q.get_ptr();
}

TEST_CASE("root1"){
    collector &gc = *root_base::get_garbage_collector();
    root<int> a;
    CHECK(gc.get_registry().size() == 1);
    {   
        root<int> rr = new int(3);
        CHECK(gc.get_registry().size() == 2);
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
}
void root_test1(){
    root<int> rr = new int(3);
    struct t {
        int a;
        char b;
        bool c;
        t(int a, char b, bool c): a(a), b(b), c(c) {}
    };
    root<struct t> r = new struct t(1, 'a', true);
    cout << (*r).a << endl << r->b << endl;
}

void root_test2(){
    collector &gc = *root_base::get_garbage_collector();
    root<int> a = new int [10];
    for(int i = 0; i < 10; ++i){
        a[i] = i+1;//*(a+i)
        cout << a[i] << endl;
    }
    cout << "print_registry()" << endl;
    gc.print_registry();
    cout << a[5] << endl;
    a = new int;
    *a = 3;
    cout << *a << std::endl;

}

void root_test3(){
    collector &gc = *root_base::get_garbage_collector();
    {
    root<int> a = new int(6);
    // a = nullptr;
    root<int> b = new int(2);{
    root<int> bb = b;

    gc.print_registry();

}
    
    gc.print_registry();
    
    
}
root<int> b = new int(4);

gc.print_registry();
    
}