# v6.0.21 Mesh material safe output fix

This patch addresses the case where `THREECPP_FORCE_UNLIT=1` renders meshes, but the normal MeshStandard/MeshPhong/MeshLambert path does not render while line primitives remain visible.

Changes:
- Material debug mode now outputs raw `vec4` color without tone mapping or color-space conversion.
- Fragment shaders sanitize NaN/Inf lighting output.
- Mesh material final color now has a small baseColor visibility fallback, preventing an invalid PBR/IBL/shadow branch from producing fully invisible fragments.
- Alpha is clamped and given a diagnostic minimum in opaque material paths.

This keeps geometry/VAO/draw behavior unchanged and isolates the issue to the material shader path.
