#include <atomic>
#include <thread>
#include <cassert>

// Fixed-Size FIFO (Single Producer, Single Consumer)
template<typename T>
class SPSCQueue {
private:
    T* buffer;
    size_t capacity;

    static const size_t CACHE_LINE = std::hardware_destructive_interference_size;

    // Ensure size_t is Lock Free on this platform
    static_assert(std::atomic<size_t>::is_always_lock_free);

    // Index for Pushing Objects to the Back of the Queue
    alignas(CACHE_LINE) std::atomic<size_t> back_index;
    // Padding to ensure no false sharing occurs
    char __padding[CACHE_LINE - sizeof(std::atomic<size_t>)];
    // Index for Popping Objects from the Front of the Queue
    alignas(CACHE_LINE) std::atomic<size_t> front_index;
    
public:
    // Capacity must be a Power of 2
    SPSCQueue(size_t _capacity = (1 << 12)) :
        buffer(nullptr),
        capacity(_capacity),
        back_index(0u),
        front_index(0u) {
        // Verify Correct Parameters
        assert( ((capacity & (capacity - 1)) == 0) && "SPSCQueue(_capacity): _capacity must be a power of 2!" );

        // Allocate Buffer
        buffer = new T[capacity];
    }

    ~SPSCQueue() {
        // Free Buffer
        delete[] buffer;
    }

    // Tries to Push an Item onto the Back of the Queue
    // Returns True if Successful
    bool Push(const T& item) {
        size_t back = back_index.load(std::memory_order_relaxed);
        size_t front = front_index.load(std::memory_order_acquire); // Producer must see updated front

        if (((back + 1) & (capacity - 1)) == front)
            return false; // Queue is Full

        // Copy the Item
        buffer[back] = item;
        
        // Increment Back Index and Store it
        back = (back + 1) & (capacity - 1);
        back_index.store(back, std::memory_order_release); // Consumer will now see updated back

        return true;
    }

    // Tries to Pop an Item from the Front of the Queue
    // Returns True if Successful
    bool Pop(T& out) {
        size_t front = front_index.load(std::memory_order_relaxed);
        size_t back = back_index.load(std::memory_order_acquire); // Consumer must see updated back

        if (back == front)
            return false; // Queue is Empty

        out = buffer[front];

        // Increment Front Index and Store it
        front = (front + 1) & (capacity - 1);
        front_index.store(front, std::memory_order_release); // Producer will now see updated front

        return true;
    }
};
