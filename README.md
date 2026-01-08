# Blast-UE-Plugin (UE4.27)

NVIDIA Blast destruction plugin for **Unreal Engine 4.27** with PhysX physics.

[![Blast Plugin Demo](https://img.youtube.com/vi/t8LBnQu4W1M/0.jpg)](https://www.youtube.com/watch?v=t8LBnQu4W1M)

> 🎬 **[Watch the Demo Video](https://www.youtube.com/watch?v=t8LBnQu4W1M)**

> **Looking for UE5?** See the [main branch](../../tree/main) for UE5/Chaos support.

## Other Supported Versions

| UE Version | Branch |
|------------|--------|
| UE 5.7 | `5.7-android-support` |
| UE 5.5 | `UE5-android-support` |
| UE 5.4 | `5.4-android-support` |
| UE 5.1 | `5.1-android-support` |
| UE 4.27 | `4.27-android-support` (this branch) |

## Supported Platforms

| Platform | Status | Notes |
|----------|--------|-------|
| Windows (x64) | ✅ Full | All features supported |
| Linux (x64) | ✅ Full | All features supported |
| Android (ARM64) | ✅ Full | Stress solver disabled (SSE intrinsics not available on ARM) |

## Installation

1. Copy the plugin folder to your project's `Plugins/` directory
2. Enable the `Blast` plugin in your `.uproject`:
```json
{
    "Name": "Blast",
    "Enabled": true
}
```
3. If your engine has the experimental `BlastPlugin`, disable it to avoid conflicts:
```json
{
    "Name": "BlastPlugin",
    "Enabled": false
}
```

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
bBuildForArmV7=False
```

### For 4.27-plus-meta Source Users

If you're building from the [4.27-plus-meta](https://github.com/uno1982/UnrealEngine/tree/4.27-plus-meta) source branch, use the following SDK/NDK versions:

| Component | Version |
|-----------|---------|
| JDK | OpenJDK 17.0.6 |
| Android SDK Platform | android-29 |
| NDK | 27.2.12479018 |

## Credits

Based on [NVIDIAGameWorks/Blast](https://github.com/NVIDIAGameWorks/Blast) SDK.
