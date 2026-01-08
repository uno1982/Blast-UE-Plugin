# Blast-UE-Plugin (UE5 Android Support)

NVIDIA Blast destruction plugin for **Unreal Engine 5** with full Android ARM64 support.

[![Blast Plugin Demo](https://img.youtube.com/vi/t8LBnQu4W1M/0.jpg)](https://www.youtube.com/watch?v=t8LBnQu4W1M)

> 🎬 **[Watch the Demo Video](https://www.youtube.com/watch?v=t8LBnQu4W1M)**

## Supported Engine Versions

| UE Version | Branch | Status |
|------------|--------|--------|
| UE 5.7 | `5.7-android-support` | ✅ Full |
| UE 5.5 | `UE5-android-support` | ✅ Full |
| UE 5.4 | `5.4-android-support` | ✅ Full |
| UE 5.1 | `5.1-android-support` | ✅ Full |
| UE 4.27 | `4.27-android-support` | ✅ Full |

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

## Android Notes

Android ARM64 is fully supported. All pre-built libraries are included.

### Build Requirements

| Requirement | Version | Notes |
|-------------|---------|-------|
| NDK | r26b (26.1.10909125) | Required for UE 5.5 |
| NDK API Level | android-29 | Minimum required |
| JDK | Java 17 | Required for UE 5.5 Gradle |

Configure these in UE Project Settings > Platforms > Android SDK.

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

## Credits

Based on [NVIDIA-Omniverse/PhysX](https://github.com/NVIDIA-Omniverse/PhysX) Blast SDK.
