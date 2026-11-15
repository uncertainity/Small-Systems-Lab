#include <iostream>
#include <vector>
#include <utility>
#include <initializer_list>
#include <cstddef>
#include <cstring>
#include <cassert>
#include <new>
#include <utility>
#include <cassert>
#include <string>
#include <stdexcept>
#include <cstdint>
#include <type_traits>

using namespace std;

class BumpArena{
    private:
        struct DestructionRecord{
            void* ptr;
            void(*destroy)(void*);
        };
        byte* memory_;
        size_t size_;
        size_t capacity_;
        size_t offset_;
        DestructionRecord* records_;
    
    public:
        BumpArena(size_t capacity){
            capacity_ = capacity;
            size_ = 0;
            offset_ = 0;
            memory_ = static_cast<byte*>(::operator new(capacity_ ));
            try {
                records_ = new DestructionRecord[capacity_];
            }
            catch (...) {
                ::operator delete(memory_);
                throw;
            }
        }


        BumpArena(const BumpArena &other) = delete;
        BumpArena& operator=(const BumpArena &other) = delete;

        BumpArena(BumpArena &&other) noexcept{
            capacity_ = other.capacity_;
            size_ = other.size_;
            memory_ = other.memory_;
            records_ = other.records_;
            offset_ = other.offset_;

            other.size_ = 0;
            other.capacity_ = 0;
            other.offset_ = 0;
            other.memory_ = nullptr;
            other.records_ = nullptr;
        }

        BumpArena& operator=(BumpArena &&other) noexcept{
            if(this == &other) return *this;
            
            for (size_t i = this->size_; i > 0; i--) {
                this->records_[i - 1].destroy(this->records_[i - 1].ptr);
            }

            delete[]this->records_;
            // for(size_t i = 0; i < this->size_;i++){
            //     Event* event = reinterpret_cast<Event*>(this->memory_ + i * sizeof(Event));
            //     event->~Event();
            // }
            ::operator delete(this->memory_);
            this->capacity_ = other.capacity_;
            this->size_ = other.size_;
            this->offset_ = other.offset_;
            this->memory_ = other.memory_;
            this->records_ = other.records_;

            other.size_ = 0;
            other.offset_ = 0;
            other.capacity_ = 0;
            other.memory_ = nullptr;
            other.records_ = nullptr;

            return *this;
        }
        template<class T>
        static void destroyRecords(void* ptr){
            T* T_ptr = static_cast<T*>(ptr);
            T_ptr->~T();
        }

        template<typename T, typename... Args>
        T* create(Args&&... args){
            // if(size_ == capacity_){
            //     cerr<<"Arena is full.\n";
            //     return nullptr;
            // }
            static_assert(alignof(T) <= alignof(std::max_align_t),
              "Over-aligned types are not supported");
            size_t alignment = alignof(T);
            size_t padding = (alignment - (offset_ % alignment)) % alignment;
            
            if (padding > capacity_ - offset_ ||
                sizeof(T) > capacity_ - offset_ - padding) {
                cerr << "Arena is full.\n";
                return nullptr;
            }
            
            byte* location = memory_ + offset_ + padding;
            T* ptr = new(location) T(forward<Args>(args)...);
            records_[size_].ptr = ptr;
            records_[size_].destroy = &destroyRecords<T>;
            size_ ++;
            offset_ += padding + sizeof(T);
            return ptr;
        }

        ~BumpArena(){
            for(size_t i = size_; i > 0; i--){
                records_[i - 1].destroy(records_[i - 1].ptr);
            }
            delete[] records_;
            ::operator delete(memory_);

        }
    };


    struct Event {
        int data;
        int time;
    
        Event(int d, int t) {
            data = d;
            time = t;
        }
    };
    
    // Tracks destructor execution
    struct Tracked {
        int id;
    
        static int destroyed[32];
        static int count;
    
        Tracked(int i) {
            id = i;
        }
    
        ~Tracked() {
            destroyed[count++] = id;
        }
    };
    
    int Tracked::destroyed[32] = {};
    int Tracked::count = 0;
    
    // Large object for testing capacity
    struct Big {
        char data[64];
    };
    
    // Object whose constructor throws
    struct Throwing {
        Throwing() {
            throw std::runtime_error("Construction failed");
        }
    };
    
    int main() {
    
        // TEST 1: Construct different types
        {
            BumpArena arena(256);
    
            int* a = arena.create<int>(42);
            double* b = arena.create<double>(3.14);
            Event* c = arena.create<Event>(10, 20);
            std::string* d =
                arena.create<std::string>("Hello");
    
            assert(a && b && c && d);
    
            assert(*a == 42);
            assert(*b == 3.14);
            assert(c->data == 10);
            assert(c->time == 20);
            assert(*d == "Hello");
    
            std::cout << "TEST 1: Mixed types PASS\n";
        }
    
        // TEST 2: Check alignment
        {
            BumpArena arena(128);
    
            char* a = arena.create<char>('A');
            double* b = arena.create<double>(2.5);
            int* c = arena.create<int>(100);
    
            assert(a && b && c);
    
            assert(reinterpret_cast<uintptr_t>(b)
                   % alignof(double) == 0);
    
            assert(reinterpret_cast<uintptr_t>(c)
                   % alignof(int) == 0);
    
            assert(*a == 'A');
            assert(*b == 2.5);
            assert(*c == 100);
    
            std::cout << "TEST 2: Alignment PASS\n";
        }
    
        // TEST 3: Arena capacity overflow
        {
            BumpArena arena(4);
    
            Big* a = arena.create<Big>();
    
            assert(a == nullptr);
    
            // Failed allocation must not consume space
            int* b = arena.create<int>(50);
    
            assert(b != nullptr);
            assert(*b == 50);
    
            std::cout << "TEST 3: Capacity overflow PASS\n";
        }
    
        // TEST 4: Destructor execution order
        {
            Tracked::count = 0;
    
            {
                BumpArena arena(128);
    
                arena.create<Tracked>(1);
                arena.create<Tracked>(2);
                arena.create<Tracked>(3);
            }
    
            assert(Tracked::count == 3);
    
            // Reverse construction order
            assert(Tracked::destroyed[0] == 3);
            assert(Tracked::destroyed[1] == 2);
            assert(Tracked::destroyed[2] == 1);
    
            std::cout << "TEST 4: Destructor order PASS\n";
        }
    
        // TEST 5: Move constructor
        {
            BumpArena a(256);
    
            Event* event = a.create<Event>(100, 200);
    
            BumpArena b(std::move(a));
    
            // Existing pointer should remain valid
            assert(event->data == 100);
            assert(event->time == 200);
    
            // Moved-to arena should still accept objects
            int* x = b.create<int>(99);
    
            assert(x != nullptr);
            assert(*x == 99);
    
            std::cout << "TEST 5: Move constructor PASS\n";
        }
    
        // TEST 6: Move assignment
        {
            Tracked::count = 0;
    
            {
                BumpArena a(128);
                BumpArena b(128);
    
                a.create<Tracked>(10);
    
                Tracked* p = b.create<Tracked>(20);
                b.create<Tracked>(30);
    
                a = std::move(b);
    
                // Old object in a was destroyed
                assert(Tracked::count == 1);
                assert(Tracked::destroyed[0] == 10);
    
                // Objects formerly in b still exist
                assert(p->id == 20);
    
                // Self-move assignment
                BumpArena& ref = a;
                a = std::move(ref);
    
                assert(p->id == 20);
            }
    
            assert(Tracked::count == 3);
            assert(Tracked::destroyed[1] == 30);
            assert(Tracked::destroyed[2] == 20);
    
            std::cout << "TEST 6: Move assignment PASS\n";
        }
    
        // TEST 7: Exception safety
        {
            BumpArena arena(sizeof(int));
    
            bool caught = false;
    
            try {
                arena.create<Throwing>();
            }
            catch (const std::runtime_error&) {
                caught = true;
            }
    
            assert(caught);
    
            // Failed construction should not advance offset
            int* p = arena.create<int>(123);
    
            assert(p != nullptr);
            assert(*p == 123);
    
            std::cout << "TEST 7: Exception safety PASS\n";
        }
    
        // TEST 8: Copy operations disabled
        {
            static_assert(
                !std::is_copy_constructible_v<BumpArena>
            );
    
            static_assert(
                !std::is_copy_assignable_v<BumpArena>
            );
    
            std::cout << "TEST 8: Copy disabled PASS\n";
        }
    
        std::cout << "\nALL TESTS PASSED!\n";
    
        return 0;
    }

//g++ -std=c++17 -O1 -g -Wall -Wextra -fsanitize=address,undefined -fno-omit-frame-pointer arena_alloc.cpp -o arena