# v6.0.12 Fast-start examples

This patch fixes startup freezes on macOS examples such as 13_v2_0_threejs_stack, 14_cache_dashed_texture_transform, 15_skinning_animation_test, and 16_pbr_material_gallery.

## Cause

Many general-purpose examples were synchronously building high-quality CPU PMREM environments before the first visible frame. On macOS the window could remain invisible while the main thread was busy.

## Fix

- PMREMOptions defaults are now preview friendly: cubeSize=64, mipLevels=6, irradianceSamples=32, prefilterSamples=32.
- General examples use preview PMREM instead of 256px high-quality PMREM.
- Reported examples call `window.poll()` immediately after window creation so the OS can show the window before CPU setup.
- Large procedural environment textures in non-PMREM examples were reduced to 256x128.

Dedicated PMREM quality examples may still raise PMREM options when intentionally profiling IBL precision.
