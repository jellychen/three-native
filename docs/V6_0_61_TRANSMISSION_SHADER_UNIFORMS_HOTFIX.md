# v6.0.61 Transmission shader uniform hotfix

## Problem

Running a physical/transmission scene could crash during shader compilation with errors like:

```text
GL shader compile failed:
Use of undeclared identifier 'transmissionResolution'
Use of undeclared identifier 'transmissionDebugMode'
```

The model warning below is non-fatal and only means the Assimp viewer did not receive a model path:

```text
Assimp load failed: Unable to open file "assets/models/model.glb".
Falling back to cube. Pass a model path as argv[1].
```

The real crash was the shader compile failure after fallback rendering started.

## Root cause

`ShaderLib.cpp` declared `transmissionResolution` and `transmissionDebugMode` only inside `#ifdef USE_TRANSMISSION_RENDERTARGET`, but the physical material fragment path also reads them in branches that are compiled whenever `USE_PHYSICAL` is enabled.

That creates a valid program-key combination in which:

- `USE_PHYSICAL` is defined;
- `USE_TRANSMISSION_RENDERTARGET` is not defined;
- the shader still references `transmissionResolution` / `transmissionDebugMode`.

This is the same class of state split three.js avoids by keeping renderer-level physical/transmission parameters available to the material path even when the render target path is disabled or unavailable.

## Fix

Moved these uniforms to the unconditional `USE_PHYSICAL` scope:

```glsl
uniform vec2 transmissionResolution;
uniform int transmissionDebugMode;
```

Kept actual render-target resources guarded:

```glsl
#ifdef USE_TRANSMISSION_RENDERTARGET
uniform sampler2D transmissionSamplerMap;
uniform sampler2D transmissionBackfaceMap;
uniform float transmissionSamplerMapLevel;
uniform float transmissionCameraFar;
uniform int transmissionUseBackfaceMap;
#endif
```

`GLRenderer::uploadUniforms()` already uploads safe defaults for the two uniforms, using the transmission target size when it exists and the viewport size otherwise.

## Expected behavior

- `06_physical_transmission` should no longer crash due to missing transmission uniforms.
- `02_assimp_viewer` may still print the missing default model warning if no argv model path is supplied, but fallback cube rendering should continue normally.
- Passing an explicit model path remains supported:

```bash
xmake run 02_assimp_viewer /path/to/model.glb
```
