# Blast-UE-Plugin (UE5)

NVIDIA Blast plugin for **Unreal Engine 5** (Chaos physics compatible)

> **Looking for UE4.27?** See the [UE4 branch](../../tree/ue4) or use the standalone UE4 plugin.

## Supported Platforms

| Platform | Status | Notes |
|----------|--------|-------|
| Windows (x64) | ✅ Full | All features supported |
| Linux (x64) | ✅ Full | All features supported |
| Android (ARM64) | ⚠️ In Progress | Requires SDK-matched libraries |

## SDK Version

This plugin uses **Blast SDK 5.x** from [NVIDIA-Omniverse/PhysX](https://github.com/NVIDIA-Omniverse/PhysX).

The pre-built libraries in `Libraries/` are from:
- Blast SDK 5.x (branch: `blast-sdk-5.5.1`)
- Cap'n Proto v0.6.1 (built with `CAPNP_LITE=1` for Android)

## Android Notes

Android ARM64 support considerations:

### What Works
- Core Blast fracture simulation
- Damage programs (radial, shear, capsule, etc.)
- Asset serialization (Cap'n Proto with CAPNP_LITE)
- Glue volumes and extended support actors

### Limitations
- **Stress Solver**: Disabled on Android. The stress solver uses x86 SSE intrinsics (`__m128`, `_mm_*` functions) which are not available on ARM. If `bCalculateStress` is enabled on a BlastMeshComponent, stress calculations will be skipped.

### Building Android Libraries

Build requirements:
- Android NDK 25.1.8937393+
- CMake 3.22+
- Target: `arm64-v8a`, API level 24+

## UE4.27 Compatibility

This plugin is designed for **UE5**. For UE4.27:
- Use the original [NVIDIAGameWorks/Blast](https://github.com/NVIDIAGameWorks/Blast) SDK (older API)
- The UE4 plugin requires libraries built from the matching SDK version
- SDK versions are **not interchangeable** due to ABI differences
