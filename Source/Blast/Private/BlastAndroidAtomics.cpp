// Copyright (c) 2024 NVIDIA Corporation. All rights reserved.
// Platform-specific atomic implementations for Android
// These symbols are expected by the Blast static libraries but not defined in them

#if PLATFORM_ANDROID

namespace Nv
{
namespace Blast
{

// Atomically increment value and return the new value
int atomicIncrement(volatile int* val)
{
    return __sync_add_and_fetch(val, 1);
}

// Atomically decrement value and return the new value  
int atomicDecrement(volatile int* val)
{
    return __sync_sub_and_fetch(val, 1);
}

} // namespace Blast
} // namespace Nv

#endif // PLATFORM_ANDROID
