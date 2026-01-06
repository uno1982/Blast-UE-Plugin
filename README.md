# Blast-UE-Plugin
NVIDIA Blast plugin for Unreal Engine (PhysX and Chaos compatible)

## Supported Platforms

| Platform | Status | Notes |
|----------|--------|-------|
| Windows (x64) | ✅ Full | All features supported |
| Linux (x64) | ✅ Full | All features supported |
| Android (ARM64) | ✅ Partial | Stress solver disabled (SSE intrinsics not available on ARM) |

## Android Notes

Android ARM64 support is included with the following considerations:

### What Works
- Core Blast fracture simulation
- Damage programs (radial, shear, capsule, etc.)
- Asset serialization (Cap'n Proto with CAPNP_LITE)
- Glue volumes and extended support actors

### Limitations
- **Stress Solver**: Disabled on Android. The stress solver uses x86 SSE intrinsics (`__m128`, `_mm_*` functions) which are not available on ARM. If `bCalculateStress` is enabled on a BlastMeshComponent, a warning will be logged and stress calculations will be skipped.

### Building Android Libraries

The pre-built Android libraries in `Libraries/Android/release/` were built from:
- [NVIDIA-Omniverse/PhysX](https://github.com/NVIDIA-Omniverse/PhysX) - Blast SDK 5.x (branch: `blast-sdk-5.5.1`)
- [Cap'n Proto v0.6.1](https://github.com/capnproto/capnproto) - Built with `CAPNP_LITE=1`

Build requirements:
- Android NDK 25.1.8937393+
- CMake 3.22+
- Target: `arm64-v8a`, API level 24+
