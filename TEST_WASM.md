# WASM Build Test Environment

## Quick Start

Build the WASM version to verify compilation:

```bash
cd /path/to/xash3d-fwgs
docker compose -f docker-compose.yml up --build
```

Or using docker directly:

```bash
docker build -t xash3d-fwgs-wasm -f Dockerfile.test .
```

## What This Tests

This Dockerfile tests that the xash3d-fwgs codebase compiles successfully under emscripten/WASM, which uses C++ compilation. It specifically verifies:

1. **C++ Compatibility Fixes**: The `platform.h` forward enum declarations are properly guarded with `#ifndef __cplusplus`
2. **VGUI2 Bootstrap Code**: The new engine/client/vgui2/ C++ code compiles correctly
3. **SDL2 Integration**: WASM-compatible SDL2 build settings

## Troubleshooting

### Forward Enum Errors
If you see errors like:
```
ISO C++ forbids forward references to 'enum' types
```

The fix is in `engine/platform/platform.h`:
```c
#ifndef __cplusplus
typedef enum window_mode_e window_mode_t;
typedef enum ref_window_type_e ref_window_type_t;
typedef enum ref_graphic_apis_e ref_graphic_apis_t;
#endif
```

### Other C++ Compatibility Issues
The codebase has these patterns that are already guarded:
- `public/xash3d_mathlib.h` - Compound literals (`vec3_origin`) guarded
- `public/xash3d_mathlib.h` - Union type punning (`float_bits_t`) guarded

## Files

- `Dockerfile.test` - Multi-stage build for compilation verification
- `docker-compose.yml` - Docker Compose configuration
