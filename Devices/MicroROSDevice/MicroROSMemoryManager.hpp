/*******************************************************************************
* Copyright (c) 2025.
 * IWIN-FINS Lab, Shanghai Jiao Tong University, Shanghai, China.
 * All rights reserved.
 ******************************************************************************/

#ifndef MICROROS_MEMORY_MANAGER_HPP
#define MICROROS_MEMORY_MANAGER_HPP

#include <rcl/allocator.h>
#include <rcutils/allocator.h>

/**
 * @class MicroROSMemoryManager
 * @brief Manages a dedicated static memory pool for the micro-ROS application.
 *
 * This class implements a simple "pointer bump" allocator on a static memory
 * block. This is the most fundamental way to achieve static memory allocation
 * for micro-ROS, ensuring deterministic behavior without external dependencies.
 */
class MicroROSMemoryManager
{
public:
    /**
     * @brief Initializes the static memory allocator.
     *        This must be called once before getAllocator() is used.
     */
    static void initialize();

    /**
     * @brief Retrieves a pointer to the pre-configured static allocator.
     * @return A pointer to the rcl_allocator_t instance for static memory.
     */
    static rcl_allocator_t* getAllocator();

private:
    // Prevent instantiation
    MicroROSMemoryManager() = delete;

    // --- Custom Allocator Functions ---
    // These functions will be the implementation for our static allocator.
    static void* static_allocate(size_t size, void* state);
    static void static_deallocate(void* pointer, void* state);
    static void* static_reallocate(void* pointer, size_t size, void* state);
    static void* static_zero_allocate(size_t number_of_elements, size_t size_of_element, void* state);

    // --- State ---
    static bool is_initialized_;
    static rcl_allocator_t rcl_allocator_;
};

#endif // MICROROS_MEMORY_MANAGER_HPP