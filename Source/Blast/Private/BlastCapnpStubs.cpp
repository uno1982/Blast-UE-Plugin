// Copyright (c) 2024 NVIDIA Corporation. All rights reserved.
// Cap'n Proto lite mode stub implementations for Android
// When CAPNP_LITE=1, some KJ functions may be missing from the library

#if PLATFORM_ANDROID

#include "CoreMinimal.h"
#include <cstdlib>

namespace kj
{

// ThrowOverflow is a functor class in kj used for bounds checking
// In lite mode, this may be missing. We provide a definition that crashes.
class ThrowOverflow
{
public:
    // Use __attribute__((used)) to prevent the compiler from optimizing away
    [[noreturn]] __attribute__((used, noinline)) void operator()() const
    {
        UE_LOG(LogTemp, Fatal, TEXT("Cap'n Proto: Integer overflow detected"));
        std::abort();
    }
};

// Force the symbol to be emitted by taking a pointer to the method
__attribute__((used)) static void (ThrowOverflow::*s_ThrowOverflowOp)() const = &ThrowOverflow::operator();

} // namespace kj

#endif // PLATFORM_ANDROID
