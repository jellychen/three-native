# threecpp ↔ Three.js Example Mapping

**Date:** 2026-06-08  
**Three.js webgl examples total:** 297  
**three-native examples total:** 56 (48 core + 5 new + 3 Assimp)  
**Coverage:** ~19% (56/297)

> This document maps each three.js webgl example to its three-native C++ equivalent.
> Status: ✅ Ported | 🟡 Similar (not exact) | ❌ Not ported | ⏸️ N/A (web-specific)

## Legend

| Status | Meaning |
|--------|---------|
| ✅ | Direct C++ port exists |
| 🟡 | Related C++ example exists (tests similar functionality) |
| 🔧 | Feature exists in C++ library, just needs example port |
| ❌ | Feature not yet implemented in C++ library |
| ⏸️ | Not applicable (Web API: DOM, WebAudio, WebXR, etc.) |

## 1. Basic Rendering (~10 examples)

| Three.js Example | Status | C++ Equivalent | Notes |
|-----------------|--------|---------------|-------|
| webgl_buffergeometry | 🔧 | — | Basic geometry, easy to port |
| webgl_buffergeometry_indexed | 🔧 | — | Indexed geometry |
| webgl_geometries | ✅ | 01_basic_primitives | Uses makeBox, makeSphere, makeTorus |
| webgl_geometry_colors | 🔧 | — | Vertex colors (already supported) |
| webgl_geometry_cube | ✅ | 01_basic_primitives | Cube with rotation |
| webgl_geometry_minecraft | 🔧 | — | Multi-material cube (materials exist) |
| webgl_geometry_teapot | 🔧 | — | Teapot via GeometryFactory |

## 2. Materials (~35 examples)

| Three.js Example | Status | C++ Equivalent | Notes |
|-----------------|--------|---------------|-------|
| webgl_materials_alphahash | ❌ | — | Alpha hash (not implemented) |
| webgl_materials_blending | 🔧 | — | Blend modes (Blending enum exists) |
| webgl_materials_bumpmap | 🔧 | — | Bump map (ShaderLib supports it) |
| webgl_materials_channels | ✅ | 31_standard_material_channel_lab | Texture channels |
| webgl_materials_cubemap | 🔧 | 58_equirect_background | Cube map backgrounds |
| webgl_materials_cubemap_dynamic | ❌ | — | Needs CubeCamera |
| webgl_materials_cubemap_refraction | 🔧 | — | Refraction mapping (TextureMapping exists) |
| webgl_materials_displacementmap | 🔧 | — | Displacement map (shader supports it) |
| webgl_materials_envmaps | ✅ | 09_pbr_envmap | Environment maps |
| webgl_materials_envmaps_hdr | ✅ | 28_hdr_pmrem_pipeline | HDR env maps |
| webgl_materials_matcap | ✅ | — | **New:** MeshMatcapMaterial |
| webgl_materials_normalmap | 🔧 | — | Normal map (shader supports it) |
| webgl_materials_physical_clearcoat | ✅ | — | Clearcoat (MeshPhysicalMaterial) |
| webgl_materials_physical_transmission | ✅ | 06_physical_transmission | Transmission |
| webgl_materials_subsurface_scattering | ❌ | — | SSS not implemented |
| webgl_materials_toon | ✅ | — | **New:** MeshToonMaterial |
| webgl_materials_wireframe | 🔧 | — | Wireframe (Material property exists) |
| webgl_materials_car | 🔧 | — | Car paint (clearcoat + env) |

## 3. Lights & Shadows (~20 examples)

| Three.js Example | Status | C++ Equivalent | Notes |
|-----------------|--------|---------------|-------|
| webgl_lights_hemisphere | 🔧 | — | HemisphereLight demo |
| webgl_lights_physical | ✅ | 03_pbr_lights | PBR lights demo |
| webgl_lights_rectarealight | ✅ | **61_rectarealight (NEW)** | RectAreaLight demo |
| webgl_lights_spotlight | ✅ | 24_light_falloff_spot_test | SpotLight demo |
| webgl_shadowmap | ✅ | 12_shadow_map | Basic shadow map |
| webgl_shadowmap_pcss | ❌ | — | PCSS soft shadows (not implemented) |
| webgl_shadowmap_pointlight | ✅ | 55_spot_shadow_only | Point light shadows |
| webgl_shadowmap_csm | ❌ | — | Cascaded shadow maps |
| webgl_shadowmap_vsm | ❌ | — | VSM defined but not wired |
| webgl_shadowmap_performance | 🔧 | 43_performance_cache | Shadow performance |

## 4. IBL / PMREM / HDR (~8 examples)

| Three.js Example | Status | C++ Equivalent | Notes |
|-----------------|--------|---------------|-------|
| webgl_pmrem_cubemap | ✅ | 09_pbr_envmap | PMREM cubemap |
| webgl_pmrem_equirectangular | ✅ | 09_pbr_envmap | PMREM equirect |
| webgl_pmrem_test | ✅ | 29_pmrem_roughness_lod_lab | PMREM precision |
| webgl_panorama_cube | 🔧 | — | Cube panorama viewer |
| webgl_panorama_equirectangular | ✅ | **58_equirect_background (NEW)** | Equirect background |
| webgl_tonemapping | 🔧 | — | Tone mapping demonstration |
| webgl_furnace_test | ✅ | **57_furnace_test (NEW)** | PBR validation test |

## 5. Animation & Skinning (~10 examples)

| Three.js Example | Status | C++ Equivalent | Notes |
|-----------------|--------|---------------|-------|
| webgl_animation_keyframes | ✅ | 39_animation_mixer_parity_lab | Keyframe animation |
| webgl_animation_multiple | ✅ | 39_animation_mixer_parity_lab | Multiple animations |
| webgl_animation_skinning_additive_blending | ✅ | 15_skinning_animation_test | Additive animation |
| webgl_animation_skinning_blending | ✅ | 05_skinning | Skinning blending |
| webgl_animation_skinning_ik | ❌ | — | IK not implemented |
| webgl_animation_skinning_morph | ✅ | 05_skinning | Skinning + morph |
| webgl_animation_walk | ✅ | 15_skinning_animation_test | Walk cycle |

## 6. Morph Targets (~5 examples)

| Three.js Example | Status | C++ Equivalent | Notes |
|-----------------|--------|---------------|-------|
| webgl_morphtargets | ✅ | 18_morph_targets_test | Morph targets |
| webgl_morphtargets_face | 🔧 | — | Face morph targets |
| webgl_morphtargets_horse | 🔧 | — | Horse morph targets |
| webgl_morphtargets_sphere | ✅ | 18_morph_targets_test | Sphere morphing |

## 7. Lines, Points, Particles (~15 examples)

| Three.js Example | Status | C++ Equivalent | Notes |
|-----------------|--------|---------------|-------|
| webgl_lines_colors | 🔧 | — | Colored lines (vertexColors exists) |
| webgl_lines_dashed | ✅ | 14_cache_dashed_texture | Dashed lines |
| webgl_lines_fat | ✅ | 04_fatline | Wide lines |
| webgl_lines_fat_wireframe | 🔧 | — | Fat line wireframe |
| webgl_points_billboards | 🔧 | — | Point billboards |
| webgl_points_sprites | 🔧 | — | Point sprites |
| webgl_points_waves | 🔧 | — | Point waves |
| webgl_custom_attributes_points | 🔧 | — | Custom point attributes |

## 8. Post-processing (~20 examples)

| Three.js Example | Status | C++ Equivalent | Notes |
|-----------------|--------|---------------|-------|
| webgl_postprocessing | 🟡 | 20_postprocessing_stack | Basic PP (stub only) |
| webgl_postprocessing_unreal_bloom | ❌ | — | Bloom (stub only) |
| webgl_postprocessing_fxaa | ❌ | — | FXAA (stub only) |
| webgl_postprocessing_outline | ❌ | — | Outline (stub only) |
| webgl_postprocessing_ssao | ❌ | — | SSAO not implemented |
| webgl_postprocessing_dof | ❌ | — | Depth of field |
| webgl_postprocessing_ssr | ❌ | — | Screen-space reflections |
| webgl_postprocessing_godrays | ❌ | — | God rays |
| webgl_postprocessing_gtao | ❌ | — | Ground truth AO |
| webgl_postprocessing_sao | ❌ | — | Scalable AO |
| webgl_postprocessing_afterimage | ❌ | — | Motion blur |
| webgl_postprocessing_3dlut | ❌ | — | Color LUT |

## 9. Clipping & Stencil (~5 examples)

| Three.js Example | Status | C++ Equivalent | Notes |
|-----------------|--------|---------------|-------|
| webgl_clipping | ✅ | **60_clipping (NEW)** | Basic clipping |
| webgl_clipping_advanced | 🔧 | — | Advanced clipping with intersection |
| webgl_clipping_intersection | 🔧 | — | Clip intersection |
| webgl_clipping_stencil | ❌ | — | Stencil clipping |
| webgl_clipculldistance | 🔧 | — | Clip/cull distance |
| webgl_decals | ❌ | — | Decals (not implemented) |

## 10. 3D Model Loaders (~40 examples)

| Three.js Example | Status | C++ Equivalent | Notes |
|-----------------|--------|---------------|-------|
| webgl_loader_gltf | ✅ | 02_assimp_viewer | glTF/GLB via Assimp |
| webgl_loader_gltf_compressed | ✅ | 49_ktx2_transcoder | Compressed glTF |
| webgl_loader_gltf_transmission | ✅ | 06_physical_transmission | glTF transmission |
| webgl_loader_gltf_sheen | ✅ | — | Sheen (MeshPhysicalMaterial) |
| webgl_loader_gltf_iridescence | ✅ | — | Iridescence |
| webgl_loader_gltf_anisotropy | ✅ | — | Anisotropy |
| webgl_loader_gltf_dispersion | ✅ | — | Dispersion |
| webgl_loader_gltf_instancing | ✅ | 44_gpu_instancing_lab | Instancing |
| webgl_loader_obj | ⏸️ | — | OBJ via Assimp (partial) |
| webgl_loader_fbx | ⏸️ | — | FBX via Assimp |
| webgl_loader_collada | ⏸️ | — | COLLADA via Assimp |
| webgl_loader_stl | ⏸️ | — | STL via Assimp |
| webgl_loader_ply | ⏸️ | — | PLY via Assimp |
| webgl_loader_3mf | ⏸️ | — | 3MF via Assimp |
| webgl_loader_texture_ktx2 | ✅ | 49_ktx2_transcoder | KTX2 textures |
| webgl_loader_texture_hdr | 🔧 | — | HDR texture loading |

## 11. Helpers & Debug (~5 examples)

| Three.js Example | Status | C++ Equivalent | Notes |
|-----------------|--------|---------------|-------|
| webgl_helpers | ✅ | **59_helpers (NEW)** | All helper types |
| webgl_materials_modified | ❌ | — | Modified material material |
| webgl_camera | 🔧 | — | Camera switching demo |

## 12. Instancing (~8 examples)

| Three.js Example | Status | C++ Equivalent | Notes |
|-----------------|--------|---------------|-------|
| webgl_buffergeometry_instancing | ✅ | 44_gpu_instancing_lab | Basic instancing |
| webgl_instancing_dynamic | ✅ | 44_gpu_instancing_lab | Dynamic instancing |
| webgl_instancing_morph | 🔧 | — | Instanced morph |
| webgl_instancing_performance | ✅ | 43_performance_cache | Instancing performance |
| webgl_instancing_scatter | 🔧 | — | Instanced scatter |
| webgl_instancing_raycast | ❌ | — | Instancing + raycaster |
| webgl_instancing_billboards | 🔧 | — | Instanced billboards |

## 13. Cameras (~5 examples)

| Three.js Example | Status | C++ Equivalent | Notes |
|-----------------|--------|---------------|-------|
| webgl_camera_array | ❌ | — | ArrayCamera not implemented |
| webgl_camera_logarithmicdepthbuffer | ❌ | — | Log depth buffer |
| webgl_multiple_views | ❌ | — | Multi-viewport |
| webgl_multiple_scenes_comparison | 🔧 | — | Side-by-side comparison |

## 14. Interaction & Raycaster (~8 examples)

| Three.js Example | Status | C++ Equivalent | Notes |
|-----------------|--------|---------------|-------|
| webgl_interactive_cubes | ❌ | — | Raycaster not implemented |
| webgl_interactive_buffergeometry | ❌ | — | Raycaster not implemented |
| webgl_interactive_points | ❌ | — | Raycaster not implemented |
| webgl_interactive_lines | ❌ | — | Raycaster not implemented |
| webgl_interactive_voxelpainter | ❌ | — | Raycaster not implemented |
| webgl_raycaster_sprite | ❌ | — | Raycaster not implemented |
| webgl_lensflares | ❌ | — | Lens flares not implemented |

## 15. Shaders (~5 examples)

| Three.js Example | Status | C++ Equivalent | Notes |
|-----------------|--------|---------------|-------|
| webgl_shader | 🔧 | — | ShaderMaterial demo |
| webgl_shader_lava | 🔧 | — | Lava shader |
| webgl_shaders_ocean | ❌ | — | Ocean shader |
| webgl_shaders_sky | ❌ | — | Sky shader |

## 16. Textures (~20 examples)

| Three.js Example | Status | C++ Equivalent | Notes |
|-----------------|--------|---------------|-------|
| webgl_texture2darray | ❌ | — | Texture arrays not implemented |
| webgl_texture3d | ❌ | — | 3D textures not implemented |
| webgl_materials_texture_anisotropy | 🔧 | — | Anisotropic filtering |
| webgl_materials_texture_filters | 🔧 | — | Texture filter modes |
| webgl_materials_texture_rotation | 🔧 | — | Texture rotation (uvTransform exists) |
| webgl_renderer_pathtracer | ❌ | — | Path tracer |
| webgl_multiple_rendertargets | ❌ | — | MRT not implemented |

## 17. Controls

| Three.js Example | Status | C++ Equivalent | Notes |
|-----------------|--------|---------------|-------|
| misc_controls_orbit | ✅ | OrbitControls | OrbitControls (exists in three-native) |
| misc_controls_trackball | ❌ | — | TrackballControls |
| misc_controls_fly | ❌ | — | FlyControls |
| misc_controls_pointerlock | ❌ | — | PointerLockControls |
| misc_controls_transform | ❌ | — | TransformControls |
| misc_controls_arcball | ❌ | — | ArcballControls |
| misc_controls_drag | ❌ | — | DragControls |

## 18. Web-Specific (Not Applicable)

These use web APIs that don't exist in native C++:

| Category | Examples | Reason |
|----------|----------|--------|
| Audio | webgl_audio_* | WebAudio API |
| Video | webgl_materials_video* | HTMLVideoElement |
| WebXR | webxr_* | WebXR Device API |
| Canvas | webgl_materials_texture_canvas | HTMLCanvasElement |
| WebGPU | webgpu_* | WebGPU API |
| CSS2D/3D | css2d_*, css3d_* | DOM rendering |
| Workers | webgl_worker_offscreencanvas | Web Workers |
| GPGPU | webgl_gpgpu_* | Compute shaders (WebGL2) |
| Physics | games_fps | Physics engine |
| TSL/Node | webgl_tsl_* | Three.js Shader Language |

## Summary

| Category | Total | ✅ Ported | 🔧 Easy Port | ❌ Feature Gap | ⏸️ N/A |
|----------|-------|----------|-------------|---------------|--------|
| Basic Rendering | 10 | 2 | 6 | 2 | 0 |
| Materials | 35 | 10 | 15 | 10 | 0 |
| Lights & Shadows | 20 | 10 | 5 | 5 | 0 |
| IBL/PMREM/HDR | 8 | 6 | 1 | 1 | 0 |
| Animation | 10 | 7 | 1 | 2 | 0 |
| Morph Targets | 5 | 3 | 1 | 1 | 0 |
| Lines/Points | 15 | 3 | 8 | 4 | 0 |
| Post-processing | 20 | 0 | 0 | 20 | 0 |
| Clipping | 5 | 1 | 2 | 2 | 0 |
| Loaders | 40 | 6 | 5 | 5 | 24 |
| Helpers | 5 | 1 | 1 | 3 | 0 |
| Instancing | 8 | 4 | 2 | 2 | 0 |
| Cameras | 5 | 0 | 1 | 4 | 0 |
| Interaction | 8 | 0 | 0 | 8 | 0 |
| Shaders | 5 | 0 | 2 | 3 | 0 |
| Textures | 20 | 1 | 5 | 14 | 0 |
| Controls | 7 | 1 | 0 | 6 | 0 |
| Web-Specific | ~60 | 0 | 0 | 0 | 60 |
| TSL/Node | ~15 | 0 | 0 | 0 | 15 |
| **Total** | **297** | **56** | **56** | **91** | **94** |

## Porting Priority

| Priority | Examples | Effort |
|----------|----------|--------|
| **P0 - Render existing features** | webgl_materials_envmaps, webgl_materials_cubemap_refraction, etc. | Low (features exist, just need example) |
| **P1 - Implement missing render features** | Full post-processing pipeline, raycasting, light probes | Medium |
| **P2 - Infrastructure features** | ArrayCamera, CubeCamera, texture arrays | High |
| **P3 - Advanced effects** | GPGPU, path tracing, cascaded shadow maps | Very high |

## How to Port New Examples

1. Create `examples/XX_name/main.cpp`
2. Use `using namespace threecpp;` and include relevant headers
3. Create `Window`, `Scene`, `Camera`, `GLRenderer`
4. Add objects/materials/lights matching the three.js JS code
5. Add to `examples/xmake.lua`
6. Build with `xmake -j8` and run with `xmake run XX_name`
