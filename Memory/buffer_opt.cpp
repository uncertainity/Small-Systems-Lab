#include <iostream>
#include <vector>
#include <utility>
#include <initializer_list>
#include <cstddef>
#include <cstring>
#include <cassert>
#include <new>
#include <utility>


using namespace std;


// Make a small buffer class

class Buffer{
    private:
        static constexpr size_t SSO_SIZE = 16;
        union{
            char local_[SSO_SIZE];
            char* heap_;
        };
        size_t size_;


    public:
        Buffer(size_t n){
            size_ = n;
            if(n > SSO_SIZE){
                heap_ = new char[size_]{};
            }
            else{
                local_[0] = '\0';
                memset(local_, 0, SSO_SIZE);
            }
        }

        Buffer(const Buffer &other){
            size_ = other.size_;
            if(size_ > SSO_SIZE){
                heap_ = new char[size_];
                memcpy(heap_,other.heap_,size_);
            }
            else{
                memcpy(local_,other.local_,size_);
            }
        }

        Buffer& operator=(const Buffer &other){
            if(this == &other) return *this;
            if(other.size_ > SSO_SIZE){
                char* new_heap = new char[other.size_];
                memcpy(new_heap,other.heap_,other.size_);
                if(this->size_ > SSO_SIZE){
                    delete [] this->heap_;
                }
                this->heap_ = new_heap; 
        
            }
            else{
                if(this->size_ > SSO_SIZE){
                    delete [] this->heap_;
                }
                this->local_[0] = '\0';
                memcpy(this->local_,other.local_,other.size_);
            }
            this->size_ = other.size_;
            return *this;
        }

        Buffer(Buffer&& other) noexcept{
            size_ = other.size_;
            if(size_ > SSO_SIZE){
                heap_ = other.heap_;
                other.local_[0] = '\0';
                other.size_ = 0;
            }
            else{
                local_[0] = '\0';
                memcpy(this->local_,other.local_,size_);
                other.local_[0] = '\0';
                other.size_ = 0;
            }
        }

        Buffer& operator=(Buffer &&other) noexcept{
            if(this == &other) return *this;
            if(size_ > SSO_SIZE){
                delete[] this->heap_;
            }
            size_ = other.size_;
            if(other.size_ > SSO_SIZE){
                this->heap_ = other.heap_;
                other.local_[0] = '\0';
                other.size_ = 0;
            }
            else{
                this->local_[0] = '\0';
                memcpy(this->local_,other.local_,size_);
                other.local_[0] = '\0';
                other.size_ = 0;
            }
            return *this;
        }

        ~Buffer(){
            if(size_ > SSO_SIZE){
                delete[] heap_;
            }
            else{
                local_[0] = '\0';
            }
            size_ = 0;
        }
    
    char* data(){
        if(size_ > SSO_SIZE){
            return heap_;
        }
        else{
            return local_;
        }
    }
};




void fillBuffer(Buffer& b, size_t n, char value) {
    for (size_t i = 0; i < n; i++) {
        b.data()[i] = value;
    }
}

void checkBuffer(Buffer& b, size_t n, char expected) {
    for (size_t i = 0; i < n; i++) {
        assert(b.data()[i] == expected);
    }
}

int main() {

    // TEST 1: Constructor and SSO boundary
    {
        Buffer a(0);
        Buffer b(10);
        Buffer c(16);
        Buffer d(17);
        Buffer e(100);

        fillBuffer(b, 10, 'A');
        fillBuffer(c, 16, 'B');
        fillBuffer(d, 17, 'C');
        fillBuffer(e, 100, 'D');

        checkBuffer(b, 10, 'A');
        checkBuffer(c, 16, 'B');
        checkBuffer(d, 17, 'C');
        checkBuffer(e, 100, 'D');

        std::cout << "TEST 1: Constructor PASS\n";
    }

    // TEST 2: Copy constructor - small
    {
        Buffer a(10);
        fillBuffer(a, 10, 'A');

        Buffer b(a);

        checkBuffer(b, 10, 'A');

        b.data()[0] = 'X';
        assert(a.data()[0] == 'A');

        std::cout << "TEST 2: Small copy PASS\n";
    }

    // TEST 3: Copy constructor - heap
    {
        Buffer a(100);
        fillBuffer(a, 100, 'B');

        Buffer b(a);

        checkBuffer(b, 100, 'B');

        b.data()[0] = 'X';
        assert(a.data()[0] == 'B');
        assert(a.data() != b.data());

        std::cout << "TEST 3: Heap copy PASS\n";
    }

    // TEST 4: Copy assignment - all transitions
    {
        Buffer small(10);
        Buffer large(100);

        fillBuffer(small, 10, 'S');
        fillBuffer(large, 100, 'L');

        // Small -> Small
        Buffer a(5);
        a = small;
        checkBuffer(a, 10, 'S');

        // Small -> Heap
        a = large;
        checkBuffer(a, 100, 'L');

        // Heap -> Small
        a = small;
        checkBuffer(a, 10, 'S');

        // Heap -> Heap
        Buffer b(50);
        b = large;
        checkBuffer(b, 100, 'L');

        std::cout << "TEST 4: Copy assignment PASS\n";
    }

    // TEST 5: Move constructor - small
    {
        Buffer a(10);
        fillBuffer(a, 10, 'A');

        Buffer b(std::move(a));

        checkBuffer(b, 10, 'A');
        assert(a.data()[0] == '\0');

        std::cout << "TEST 5: Small move PASS\n";
    }

    // TEST 6: Move constructor - heap
    {
        Buffer a(100);
        fillBuffer(a, 100, 'B');

        char* original = a.data();

        Buffer b(std::move(a));

        assert(b.data() == original);
        checkBuffer(b, 100, 'B');
        assert(a.data()[0] == '\0');

        std::cout << "TEST 6: Heap move PASS\n";
    }

    // TEST 7: Move assignment - all transitions
    {
        // Small -> Small
        Buffer a(5);
        Buffer b(10);
        fillBuffer(b, 10, 'A');

        a = std::move(b);
        checkBuffer(a, 10, 'A');
        assert(b.data()[0] == '\0');

        // Small -> Heap
        Buffer c(100);
        fillBuffer(c, 100, 'B');
        char* original = c.data();

        a = std::move(c);
        checkBuffer(a, 100, 'B');
        assert(a.data() == original);

        // Heap -> Small
        Buffer d(8);
        fillBuffer(d, 8, 'C');

        a = std::move(d);
        checkBuffer(a, 8, 'C');

        // Heap -> Heap
        Buffer e(200);
        Buffer f(300);
        fillBuffer(f, 300, 'D');

        char* ptr = f.data();

        e = std::move(f);
        checkBuffer(e, 300, 'D');
        assert(e.data() == ptr);

        std::cout << "TEST 7: Move assignment PASS\n";
    }

    // TEST 8: Self assignment
    {
        Buffer a(100);
        fillBuffer(a, 100, 'X');

        a = a;
        checkBuffer(a, 100, 'X');

        a = std::move(a);
        checkBuffer(a, 100, 'X');

        std::cout << "TEST 8: Self assignment PASS\n";
    }

    // TEST 9: Chained assignment
    {
        Buffer a(5);
        Buffer b(10);
        Buffer c(100);

        fillBuffer(c, 100, 'Z');

        a = b = c;

        checkBuffer(a, 100, 'Z');
        checkBuffer(b, 100, 'Z');
        checkBuffer(c, 100, 'Z');

        std::cout << "TEST 9: Chained assignment PASS\n";
    }

    // TEST 10: Reuse moved-from object
    {
        Buffer a(100);
        fillBuffer(a, 100, 'A');

        Buffer b(std::move(a));

        Buffer c(10);
        fillBuffer(c, 10, 'C');

        a = c;
        checkBuffer(a, 10, 'C');
        checkBuffer(b, 100, 'A');

        std::cout << "TEST 10: Moved-from reuse PASS\n";
    }

    std::cout << "\nALL TESTS PASSED!\n";
    return 0;
}












