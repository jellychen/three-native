# v6.0.11 Startup Freeze Fix

Fixes `11_material_geometry_matrix` appearing to freeze before the window is visible on macOS.

The issue was not the render loop. The example built a 256px CPU PMREM before the first event/render loop, and the CPU GGX prefilter pass could block the main thread long enough that GLFW never displayed the window.

Changes:

- `examples/11_material_geometry_matrix` now uses a fast preview PMREM: cubeSize 64, 6 mips, 32 irradiance/prefilter samples.
- The example pumps events once before PMREM creation so the window can be shown by the OS earlier.
- The example uses framebuffer size for `renderer.setSize()`.
- `PMREMGenerator` now actually honors `PMREMOptions::prefilterSamples`; before this fix `buildPrefilterCube()` always used an internal hard-coded 192 samples.

For high quality PMREM validation, use the dedicated PMREM examples and larger PMREMOptions.
