# Blast-UE-Plugin (UE4.27)

NVIDIA Blast destruction plugin for **Unreal Engine 4.27** with PhysX physics.

> **Looking for UE5?** See the [main branch](../../tree/main) for UE5/Chaos support.

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

## Credits

Based on [NVIDIAGameWorks/Blast](https://github.com/NVIDIAGameWorks/Blast) SDK.
