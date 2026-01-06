# Blast-UE-Plugin (UE 5.1 + Android Support)

NVIDIA Blast destruction plugin for **Unreal Engine 5.1** with full Android ARM64 support.

> **Branch Note**: This is the `5.1-android-support` branch, backported from UE 5.5 to UE 5.1.

## Supported Platforms

| Platform | Status | Notes |
|----------|--------|-------|
| Windows (x64) | ✅ Full | All features supported |
| Linux (x64) | ✅ Full | All features supported |
| Android (ARM64) | ✅ Full | Stress solver disabled (SSE intrinsics not available on ARM) |

## Unreal Engine Version

**This branch targets UE 5.1**. Key API changes from UE 5.5:
- Chaos physics scene locking (custom `FScopedSceneLock_Chaos` compatibility wrapper)
- Skeletal mesh import utilities (uses `FSkeletalMeshImportData` approach)
- FBX importer API differences
- Header path changes (`SkeletalMeshSceneProxy.h`, `MaterialDomain.h`, etc.)

## Installation

1. Copy the plugin folder to your project's `Plugins/` directory
2. Enable the `Blast` plugin in your `.uproject`:
```json
{
    "Name": "Blast",
    "Enabled": true
}
```

## Usage

### Impact Damage
Impact damage is **enabled by default**. When a BlastMesh actor collides with sufficient force, it will fracture.

Key settings on `BlastMeshComponent`:
- **Bind On Hit Delegate**: Enables collision-based damage (default: `true`)
- **Impact Damage Properties**: Configure hardness, damage radius, thresholds

On `BlastMesh` asset (Blast Material section):
- **Generate Hit Events For Leaf Actors**: Enable to allow smallest chunks to continue fracturing

### Hardness Tuning
`Damage = Impulse / Hardness`. Lower hardness = easier to break. Default is 10.0.

## Android Notes

Android ARM64 is fully supported. All pre-built libraries are included.

### What Works
- Core Blast fracture simulation
- All damage programs (radial, shear, capsule, impact spread, etc.)
- Asset serialization
- Glue volumes and extended support actors
- Blueprint API

### Limitation
- **Stress Solver**: Automatically disabled on Android (`BLAST_DISABLE_STRESS_SOLVER=1`). The stress solver uses x86 SSE intrinsics which are not available on ARM. All other damage types work normally.

### Android Configuration

In `DefaultEngine.ini`, ensure ARM64 is enabled:
```ini
[/Script/AndroidRuntimeSettings.AndroidRuntimeSettings]
bBuildForArm64=True
```

## UE 5.1 Backport Changes

This branch includes the following compatibility changes for UE 5.1:

### New Files
- `Source/Blast/Private/BlastChaosCompat.h` - Chaos physics locking compatibility

### API Compatibility Fixes
- Header includes updated for UE 5.1 paths
- `TBitArray::Init()` signature compatibility
- `EAllowShrinking` enum compatibility
- `Chaos::FConvexPtr` type compatibility
- FBX import API (removed `bMapMorphTargetToTimeZero`, 4-arg `ImportFbxMorphTarget`)
- `SetNumSourceModels()` API compatibility
- `NvcVec3` struct initialization syntax
- PSO precaching APIs removed (not available in UE 5.1)
- `GetAcceleration()` API change

### Static-to-Skeletal Mesh Conversion
Implemented UE 5.1 compatible replacement for `FStaticToSkeletalMeshConverter::InitializeSkeletalMeshFromStaticMesh` which was added in UE 5.2+.

## Credits

Based on [NVIDIA-Omniverse/PhysX](https://github.com/NVIDIA-Omniverse/PhysX) Blast SDK.
