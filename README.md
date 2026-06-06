# threecpp renderer v6.0.13

Shadow draw-buffer restore fix for macOS OpenGL examples that showed only background when shadows were enabled.

## v6.0.57 update

Adds three.js parity fixes for `Object3D` weak parent links, `Scene::overrideMaterial`, and C++20 `std::span`-based API boundaries. See `docs/V6_0_57_OBJECT_PARENT_OVERRIDE_SPAN.md`.


# threecpp_renderer v3.3

C++20 renderer scaffold inspired by the architecture of three.js `WebGLRenderer`.

Stack:

- C++20
- official `glm` from GitHub / xmake package manager
- TBB
- Assimp
- GLFW
- ANGLE / OpenGL ES backend boundary

This is not a line-by-line port of three.js. It is a native C++ renderer designed to converge toward three.js rendering behavior through compatible scene, geometry, material, shader-permutation and render-list concepts.

## v0.5 scope

Compared with v0.4, this version expands the PBR material system toward three.js `MeshPhysicalMaterial`:

- `MeshStandardMaterial` remains the base metallic/roughness workflow.
- `MeshPhysicalMaterial` now exposes transmission, thickness, attenuation, IOR, specular color/intensity, clearcoat, sheen and iridescence parameters.
- Transmission/clearcoat/sheen/specular/iridescence flags are part of `ProgramKey`.
- GLSL PBR path has an approximate physical extension layer.
- Physical material texture hooks are present for transmission, thickness, clearcoat, sheen and iridescence maps.
- New example: `06_physical_transmission`.


## v3.5 focused tests

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 25_standard_physical_pmrem_shadow_lab
xmake run 26_physical_material_parity_lab
xmake run 27_shadow_completeness_lab
```

See `docs/V3_5_STANDARD_PHYSICAL_PMREM_SHADOW.md`.

## Build with xmake

```bash
xmake f -m debug --use_angle=true --angle_dir=/path/to/angle
xmake
xmake run 06_physical_transmission
```

For non-ANGLE desktop OpenGL fallback:

```bash
xmake f -m debug --use_angle=false
xmake
```


## v3.5 focused tests

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 25_standard_physical_pmrem_shadow_lab
xmake run 26_physical_material_parity_lab
xmake run 27_shadow_completeness_lab
```

See `docs/V3_5_STANDARD_PHYSICAL_PMREM_SHADOW.md`.

## Build with CMake

```bash
cmake -S . -B build -DTHREECPP_USE_ANGLE=ON -DTHREECPP_ANGLE_DIR=/path/to/angle
cmake --build build --config Debug
```

`glm` is intentionally fetched from the official repository. There is no local `mini_glm` replacement.

## Examples

- `00_clear_screen`: ANGLE window + clear screen
- `01_basic_primitives`: mesh, line segments, points and fatline
- `02_assimp_viewer`: Assimp loading entry
- `03_pbr_lights`: simplified PBR + direct lights + environment hook
- `04_fatline`: screen-space fatline test
- `05_skinning`: skeleton/skinned-mesh shader path scaffold
- `06_physical_transmission`: MeshPhysicalMaterial transmission / clearcoat / sheen demo

## Current limitations

The renderer is still not fully equivalent to three.js. Most importantly, v0.5 transmission is an approximation because the renderer does not yet implement the full three.js transmission framebuffer path with backside rendering, thickness texture sampling against a transmission render target, chromatic aberration and proper refractive background lookup.

See `docs/V0_5_PHYSICAL_MATERIAL.md` for implementation notes and the remaining parity checklist.

## v0.6 update

v0.6 adds a renderer-level transmission render target path for `MeshPhysicalMaterial`, including a half-float framebuffer texture, mipmap generation, shader-side framebuffer refraction sampling, and the `07_transmission_render_target` example. See `docs/V0_6_TRANSMISSION_RENDER_TARGET.md`.


## v0.7 GLFW + ANGLE renderer test

The package now includes a concrete GLFW + ANGLE desktop test:

```bash
cmake -S . -B build -DTHREECPP_USE_ANGLE=ON -DTHREECPP_ANGLE_DIR=/path/to/angle -DTHREECPP_ENABLE_ASSIMP=OFF
cmake --build build --config Debug
./build/08_glfw_angle_renderer_test
```

The `08_glfw_angle_renderer_test` scene exercises the renderer through a real EGL/OpenGL ES context: PBR spheres, physical transmission, grid LineSegments, Points, FatLine, resize handling, camera animation and an ESC-to-exit loop. See `docs/V0_7_GLFW_ANGLE_TEST.md`.


## v0.8 macOS GLFW Native OpenGL quick test

ANGLE is now disabled by default for the macOS smoke test. Build and run with xmake:

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake
xmake run 08_macos_glfw_native_test
```

See `docs/V0_8_MACOS_GLFW_XMAKE.md`.


## v0.9 gap closure

This package starts closing the gap toward three.js rendering capability with procedural environment fallback IBL, Lambert/Phong lit material shader paths, improved PBR environment uniforms, and scene background fallback. See `docs/V0_9_GAP_CLOSURE.md`.

## v1.0 status

v1.0 adds the first real equirectangular `scene.environment` sampling path for PBR materials, camera-frustum culling, and a new `09_pbr_envmap` example.

Run the native macOS GLFW/OpenGL test path:

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 09_pbr_envmap
```

See `docs/V1_0_RENDERER_GAP_CLOSURE.md` for the current three.js parity estimate and the next PMREM tasks.


## v1.1 notes

See `docs/V1_1_PMREM_SHADOW_ASSIMP.md`. New example: `xmake run 10_pmrem_lut`.


## v1.2

Continues closing gaps to three.js: expanded PBR material slots, geometry helpers, and material matrix examples. Ray tracing is intentionally out of scope in v1.3.0. See `docs/V1_2_THREEJS_GAP_CLOSURE.md` and `docs/V1_2_1_NO_RAYTRACING.md`.


## v1.3 ShadowMap test

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 12_shadow_map
```

This version adds realtime directional/spot shadow-map depth passes and shader PCF sampling.


## v2.0 milestone

The current package is a three.js-like realtime renderer milestone, not a full 1:1 port yet. It focuses on the realtime WebGLRenderer-equivalent path: PBR materials, environment/IBL scaffolding, shadow maps, helpers, basic animation, FatLine/Points/Line and macOS GLFW native OpenGL testing. Ray tracing is intentionally not part of this branch.

Run the integrated v2.0 stack example:

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 13_v2_0_threejs_stack
```

See `docs/V2_0_THREEJS_REALTIME_STACK.md` for the exact capability matrix and remaining work.

## v2.2 focused tests

```bash
xmake run 15_skinning_animation_test
xmake run 16_pbr_material_gallery
```

- `15_skinning_animation_test`: procedural three-bone GPU skinning + SkeletonHelper + PBR + shadows.
- `16_pbr_material_gallery`: roughness/metalness/material texture matrix + MeshPhysicalMaterial parameter gallery.

## v2.2.2 note

`SkeletonHelper` now supports `make_ref<SkeletonHelper>(skinned.get())` and `skelHelper->update()` for the procedural skinning test.


## v2.3 Assimp multi-format import

When `--enable_assimp=true` is set, the project now builds `17_assimp_multi_format_viewer`, a GLFW/OpenGL test viewer for `.fbx`, `.glb`, `.gltf`, and `.obj` assets.

```bash
xmake f -m debug --use_angle=false --enable_assimp=true
xmake -r
xmake run 17_assimp_multi_format_viewer /path/to/model.glb
```

See `docs/V2_3_ASSIMP_MULTI_FORMAT_IMPORT.md`.


## v2.3.1 xmake Assimp target fix

`17_assimp_multi_format_viewer` is now registered whenever `--enable_assimp=true` is configured. The target no longer depends on `has_package("assimp")` at parse time, so xmake can resolve/install the package normally.

Run:

```bash
xmake f -m debug --use_angle=false --enable_assimp=true
xmake -r
xmake run 17_assimp_multi_format_viewer /path/to/model.glb
```

## v2.4 morph target test

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 18_morph_targets_test
```

`v2.4` adds three.js-style `morphAttributes`, `morphTargetInfluences`, morph target shader support, and Assimp import hooks for glTF/GLB/FBX blend shapes.

## v3.3 realtime parity stack

v3.3 consolidates the renderer toward the major runtime responsibilities of three.js `WebGLRenderer`.

New examples:

```bash
# Requires Assimp. Loads GLB/glTF/FBX/OBJ, prints validation stats, and displays the model.
xmake f -m debug --use_angle=false --enable_assimp=true
xmake -r
xmake run 19_gltf_validation_suite /path/to/model.glb

# Native examples without Assimp.
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 20_postprocessing_stack
xmake run 21_v3_3_runtime_cache
```

New v3.3 modules:

- `src/validation/GltfValidationSuite.hpp`
- `src/renderer/RenderQueues.hpp`
- `src/debug/DebugViews.hpp`
- `src/postprocessing/Passes.hpp`

See `docs/V3_3_THREEJS_PARITY_ROADMAP.md` for the exact parity checklist and remaining gaps.

## v3.4 stability note

`v3.4` fixes a macOS OpenGL shader compile failure in `18_morph_targets_test` caused by missing morph target attribute/uniform declarations in the generic mesh vertex shader.


## v3.7 validation viewer

```bash
xmake f -m debug --use_angle=false --enable_assimp=true
xmake -r
xmake run 30_gltf_material_animation_validation /path/to/model.glb
```

This target is meant for real glTF/GLB/FBX/OBJ compatibility testing with material, skeleton, morph target, and animation statistics.

## v3.8 quick test

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 31_standard_material_channel_lab
```

For imported glTF/GLB validation:

```bash
xmake f -m debug --use_angle=false --enable_assimp=true
xmake -r
xmake run 30_gltf_material_animation_validation /path/to/model.glb
```

## v3.9 renderer architecture lab

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 32_renderer_architecture_lab
```

v3.9 adds separated opaque/transmissive/transparent render queues, a program cache wrapper, render state stack, VAO binding-state cache, viewport/scissor API and layers filtering.

## v4.5 Asset Import Parity

新增 `38_asset_import_parity_lab`，用于验证 glTF/GLB/FBX/OBJ 导入、贴图、PBR 扩展、骨骼、动画和 morph target。

### v4.6 AnimationMixer parity

Adds a more complete three.js-like AnimationMixer / AnimationAction layer and the `39_animation_mixer_parity_lab` example.

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 39_animation_mixer_parity_lab
```


## v4.7 Morph Target / Blend Shape Parity

Run:

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 40_morph_target_parity_lab
```

See `docs/V4_7_MORPH_TARGET_PARITY.md`.

## v5.0 performance cache / large scene

Run:

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 43_performance_cache_large_scene
```

This lab stresses shared geometry/material caches, render queue sorting, transparent/transmissive objects, moving lights, and large-scene statistics.


## v5.1 GPU Instancing

Adds real InstancedMesh rendering through instanceMatrix/instanceColor attributes, glVertexAttribDivisor, and glDrawElementsInstanced. Run `xmake run 44_gpu_instancing_lab`.

## v5.2 groups / multi-material / drawRange

Added three.js-style `BufferGeometry::groups`, `drawRange`, `Mesh` material arrays, group-split RenderList items, and grouped forward/shadow/instanced drawing. Run:

```bash
xmake run 45_groups_multimaterial_drawrange
```

## v5.3 Transparent / Transmissive Queue

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 46_transparent_transmissive_queue
```

Adds stabilized opaque / transmissive / transparent sorting behavior and an opaque-only transmission background capture path.


## v5.5 glTF extensions / KTX2 / BasisU / texture transform

New example:

```bash
xmake f -m debug --use_angle=false --enable_assimp=true
xmake -r
xmake run 48_gltf_extensions_texture_transform /path/to/model.glb
```

This version adds texture transform metadata, KTX2/BasisU compressed texture staging metadata, and a glTF extension diagnostics report. KTX2/BasisU payloads are preserved and detected; full transcoding/upload is staged for the next asset pipeline pass.

## v5.6

Adds KTX2/BasisU staging, KTX2 mip parsing, GPU compressed upload path, RGBA fallback, and `49_ktx2_transcoder_viewer`.

## v5.7

PMREM precision/cache improvements are documented in `docs/V5_7_PMREM_PRECISION_CACHE.md`.
Run `xmake run 50_pmrem_precision_lab` to test roughness/metalness IBL, clearcoat, transmission, environment rotation and PMREM cache hits.

## v5.8 Shader Chunk System

v5.8 adds `ShaderChunk` and `ShaderBuilder`, centralizes ProgramKey -> GLSL defines, and moves shared fragment helpers into named chunks. Run:

```bash
xmake run 51_shader_chunk_system_lab
```

## v6.0.49

- Fixed stb_image linker errors by adding `src/thirdparty/StbImage.cpp` with `STB_IMAGE_IMPLEMENTATION`.
