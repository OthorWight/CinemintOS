#ifndef VECTORS_H
#define VECTORS_H

// Define a size type if std::size_t is not available or desired
// For a freestanding environment, ensure this type is large enough for your needs.
// 'unsigned int' might be too small on 64-bit systems if you expect very large vectors,
// but for typical kernel use cases, it might be sufficient.
using size_t = unsigned int; // Or unsigned long, etc.

// Custom placement new and delete operators (these are standard and fine in a header)
// These are used for constructing objects in already-allocated memory.
inline void* operator new(size_t, void* p) noexcept { return p; }
inline void operator delete(void*, void*) noexcept {}

// Forward declaration for potential use within vector if T is vector itself (not strictly necessary here)
// template <typename U> class vector;

template <typename T>
class vector {
private:
    T* arr;              // Pointer to the dynamic array of T objects
    size_t current_size; // Number of elements currently constructed and stored
    size_t cap;          // Total allocated capacity (in terms of number of T elements)

    // Helper function to destroy all constructed elements in the range [first, last)
    void destroy_elements_range(T* first, T* last) {
        for (; first != last; ++first) {
            first->~T();
        }
    }

    // Helper function to destroy all elements currently in the vector
    void destroy_all_elements() {
        destroy_elements_range(arr, arr + current_size);
    }

    // Helper to allocate raw memory
    // Returns nullptr on failure if the underlying global new[] returns nullptr
    char* allocate_raw(size_t num_elements) {
        if (num_elements == 0) return nullptr; // Avoid allocating 0 bytes if sizeof(T) is non-zero
        // This will call the global ::operator new[](size_t)
        return new char[sizeof(T) * num_elements];
    }

    // Helper to deallocate raw memory
    void deallocate_raw(T* ptr_to_t_array) {
        // This will call the global ::operator delete[](void*)
        delete[] reinterpret_cast<char*>(ptr_to_t_array);
    }


public:
    // Default constructor: initialize with a small default capacity (e.g., 0 or 10)
    vector() : arr(nullptr), current_size(0), cap(0) {
        // Optionally, preallocate a small default capacity:
        // reserve(10); // Or handle allocation on first push_back
    }

    // Constructor with initial capacity
    explicit vector(size_t initial_capacity) : arr(nullptr), current_size(0), cap(0) {
        reserve(initial_capacity);
    }

    // Destructor: clean up elements and memory
    ~vector() {
        destroy_all_elements();
        deallocate_raw(arr);
        arr = nullptr; // Good practice
    }

    // Add an element to the end
    void push_back(const T& value) {
        if (current_size == cap) {
            size_t new_cap = (cap == 0) ? 10 : cap * 2; // Start with 10, then double
            reserve(new_cap); // reserve will handle allocation and moving if successful
            if (cap < new_cap && arr == nullptr && new_cap > 0) {
                // Allocation in reserve failed, or reserve wasn't called for 0 initial cap.
                // This indicates an OOM from the global new.
                // For a kernel, you might panic or have a specific error state.
                // Here, we just won't be able to push_back.
                return;
            }
        }
        // If arr is still nullptr after potential reserve (e.g. reserve(0) or failed reserve)
        // This check is a bit redundant if reserve handles allocation properly.
        if (arr == nullptr && cap > 0) {
             char* raw_mem = allocate_raw(cap);
             if (!raw_mem) return; // OOM
             arr = reinterpret_cast<T*>(raw_mem);
        } else if (arr == nullptr && cap == 0) { // First push_back, no capacity
            reserve(10); // Try to allocate default capacity
            if (!arr) return; // OOM
        }


        new (arr + current_size) T(value); // Construct new element using placement new
        ++current_size;
    }

    // Add an element to the end (move version)
    void push_back(T&& value) {
        if (current_size == cap) {
            size_t new_cap = (cap == 0) ? 10 : cap * 2;
            reserve(new_cap);
             if (cap < new_cap && arr == nullptr && new_cap > 0) return; // OOM
        }
        if (arr == nullptr && cap > 0) {
             char* raw_mem = allocate_raw(cap);
             if (!raw_mem) return; // OOM
             arr = reinterpret_cast<T*>(raw_mem);
        } else if (arr == nullptr && cap == 0) {
            reserve(10);
            if (!arr) return; // OOM
        }
        new (arr + current_size) T(static_cast<T&&>(value)); // Use placement new with std::move semantics
        ++current_size;
    }


    // Remove the last element
    void pop_back() {
        if (current_size > 0) {
            --current_size;
            (arr + current_size)->~T(); // Destroy the last element
        }
    }

    // Return the number of elements
    size_t size() const noexcept {
        return current_size;
    }

    // Return the current capacity
    size_t capacity() const noexcept {
        return cap;
    }

    bool empty() const noexcept {
        return current_size == 0;
    }

    // Element access (non-const)
    T& operator[](size_t index) {
        // Add bounds checking if desired for debugging
        // if (index >= current_size) { /* panic or error */ }
        return arr[index];
    }

    // Element access (const)
    const T& operator[](size_t index) const {
        // if (index >= current_size) { /* panic or error */ }
        return arr[index];
    }

    // Copy constructor
    vector(const vector& other) : arr(nullptr), current_size(0), cap(0) {
        if (other.cap > 0) {
            char* raw_mem = allocate_raw(other.cap);
            if (!raw_mem) { // OOM
                // Decide on behavior: leave vector empty, or indicate error
                cap = 0; current_size = 0; arr = nullptr;
                return;
            }
            arr = reinterpret_cast<T*>(raw_mem);
            cap = other.cap;
            current_size = other.current_size;
            for (size_t i = 0; i < current_size; ++i) {
                new (arr + i) T(other.arr[i]); // Deep copy elements
            }
        }
    }

    // Copy assignment operator
    vector& operator=(const vector& other) {
        if (this != &other) {
            // Strong exception guarantee approach (simplified for no exceptions)
            // Create a temporary copy
            vector temp(other); // Uses copy constructor (handles OOM there)
            // Swap contents with the temporary
            swap(*this, temp);
        }
        return *this;
    }

    // Move constructor
    vector(vector&& other) noexcept
        : arr(other.arr), current_size(other.current_size), cap(other.cap) {
        other.arr = nullptr;
        other.current_size = 0;
        other.cap = 0;
    }

    // Move assignment operator
    vector& operator=(vector&& other) noexcept {
        if (this != &other) {
            destroy_all_elements();
            deallocate_raw(arr);

            arr = other.arr;
            current_size = other.current_size;
            cap = other.cap;

            other.arr = nullptr;
            other.current_size = 0;
            other.cap = 0;
        }
        return *this;
    }

    // Remove all elements (capacity remains)
    void clear() noexcept { // noexcept if T::~T() is noexcept
        destroy_all_elements();
        current_size = 0;
    }

    // Reserve capacity for at least n elements
    void reserve(size_t n) {
        if (n > cap) {
            char* new_raw_mem = allocate_raw(n);
            if (!new_raw_mem) {
                // OOM: Failed to reserve. Vector state unchanged regarding capacity.
                // Consider how to signal this failure if needed.
                return;
            }
            T* new_arr = reinterpret_cast<T*>(new_raw_mem);

            // Move or copy existing elements
            // If T is not nothrow_move_constructible, we should copy.
            // For simplicity, we'll copy here, but move would be more efficient.
            for (size_t i = 0; i < current_size; ++i) {
                new (new_arr + i) T(static_cast<T&&>(arr[i])); // Try to move
                (arr + i)->~T(); // Destroy old element after moving
            }
            // If we just copied, we'd destroy elements after loop without moving:
            // destroy_all_elements(); // (if we only copied, not moved)

            deallocate_raw(arr); // Deallocate old raw memory
            arr = new_arr;
            cap = n;
        }
    }

    // Resize the vector
    void resize(size_t n) { // Default construct new elements
        resize(n, T());
    }

    void resize(size_t n, const T& value) { // Fill new elements with 'value'
        if (n > current_size) {
            if (n > cap) {
                reserve(n); // reserve will handle OOM
                if (cap < n) return; // OOM in reserve
            }
            // If arr is null after reserve (e.g., reserve(0) or failed reserve for n > 0)
            if (arr == nullptr && n > 0) {
                 char* raw_mem = allocate_raw(n > cap ? n : cap); // ensure enough cap
                 if(!raw_mem) return; // OOM
                 arr = reinterpret_cast<T*>(raw_mem);
                 if (n > cap) cap = n;
            }

            for (size_t i = current_size; i < n; ++i) {
                new (arr + i) T(value); // Construct new elements
            }
        } else if (n < current_size) {
            destroy_elements_range(arr + n, arr + current_size); // Destroy excess elements
        }
        current_size = n;
    }

    // Swap function
    friend void swap(vector& first, vector& second) noexcept {
        // Manual swap for each member
        T* temp_arr = first.arr;
        first.arr = second.arr;
        second.arr = temp_arr;
    
        size_t temp_current_size = first.current_size;
        first.current_size = second.current_size;
        second.current_size = temp_current_size;
    
        size_t temp_cap = first.cap;
        first.cap = second.cap;
        second.cap = temp_cap;
    }

    // Iterator support
    T* begin() noexcept { return arr; }
    const T* begin() const noexcept { return arr; }
    T* end() noexcept { return arr + current_size; }
    const T* end() const noexcept { return arr + current_size; }

    // For C++11 range-based for loops (same as begin/end)
    T* data() noexcept { return arr; } // Provides access to raw array
    const T* data() const noexcept { return arr; }
};

#endif // VECTORS_H