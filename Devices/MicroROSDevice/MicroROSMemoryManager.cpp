/*******************************************************************************
 * Copyright (c) 2025.
 * IWIN-FINS Lab, Shanghai Jiao Tong University, Shanghai, China.
 * All rights reserved.
 ******************************************************************************/

#include "MicroROSMemoryManager.hpp"
#include <cstring> // For memset
#include <cstdint>
// --- Static Member Variable Definitions ---

bool MicroROSMemoryManager::is_initialized_ = false;
rcl_allocator_t MicroROSMemoryManager::rcl_allocator_;


// --- Static Memory Pool and Management ---

constexpr size_t MICROROS_STATIC_MEMORY_SIZE = 2 * 1024; // 15 KB
static uint8_t micro_ros_static_memory[MICROROS_STATIC_MEMORY_SIZE];
static size_t current_offset = 0; // The "bump" pointer for our simple allocator


// --- Custom Allocator Function Implementations ---

void* MicroROSMemoryManager::static_allocate(size_t size, void* state)
{
    (void)state; // State is unused in this simple implementation

    // Align the allocation to a word boundary (4 bytes on 32-bit MCUs) for safety
    const size_t alignment = 4;
    size_t aligned_size = (size + alignment - 1) & ~(alignment - 1);

    if (current_offset + aligned_size > MICROROS_STATIC_MEMORY_SIZE) {
        // Not enough memory left
        return nullptr;
    }

    void* allocated_memory = &micro_ros_static_memory[current_offset];
    current_offset += aligned_size;

    return allocated_memory;
}

void MicroROSMemoryManager::static_deallocate(void* pointer, void* state)
{
    (void)state;
    (void)pointer;
    // NOTE: This is a simple pointer-bump allocator. It does not support deallocation.
    // This is a common simplification for embedded systems where all memory is
    // allocated at startup and never freed. If deallocation is needed, a more
    // complex memory manager (like a free-list allocator) would be required.
}

void* MicroROSMemoryManager::static_reallocate(void* pointer, size_t size, void* state)
{
    (void)state;
    // NOTE: Reallocation is also not supported. For micro-ROS initialization,
    // it's typically not used. We will implement a simple version that
    // might waste memory but prevents crashes.
    void* new_ptr = static_allocate(size, state);
    if (new_ptr != nullptr && pointer != nullptr) {
        // Simple realloc only copies if the new pointer is at the very end of the previous allocation.
        // A full implementation is complex. For init, this is often sufficient.
        // We will just allocate a new block and leak the old one (since deallocate does nothing).
    }
    return new_ptr;
}

void* MicroROSMemoryManager::static_zero_allocate(size_t number_of_elements, size_t size_of_element, void* state)
{
    size_t size = number_of_elements * size_of_element;
    void* ptr = static_allocate(size, state);
    if (ptr != nullptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}


// --- Public Static Member Function Implementations ---

void MicroROSMemoryManager::initialize()
{
    if (is_initialized_)
    {
        return;
    }

    // Get the default allocator structure to pre-fill some fields
    rcl_allocator_ = rcl_get_default_allocator();

    // Override the function pointers to point to our custom static implementations
    rcl_allocator_.allocate = &MicroROSMemoryManager::static_allocate;
    rcl_allocator_.deallocate = &MicroROSMemoryManager::static_deallocate;
    rcl_allocator_.reallocate = &MicroROSMemoryManager::static_reallocate;
    rcl_allocator_.zero_allocate = &MicroROSMemoryManager::static_zero_allocate;

    // State can be used to pass context to allocator functions, not needed here.
    rcl_allocator_.state = nullptr;

    is_initialized_ = true;
}

rcl_allocator_t* MicroROSMemoryManager::getAllocator()
{
    return is_initialized_ ? &rcl_allocator_ : nullptr;
}