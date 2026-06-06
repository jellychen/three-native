# v6.0.13 Shadow Draw Buffer Restore Fix

Fixes macOS OpenGL examples showing only the blue/background clear while meshes
are invisible when shadows are enabled.

Root cause: shadow target allocation/rendering used depth-only FBOs and
`glDrawBuffer(GL_NONE)` / `glReadBuffer(GL_NONE)`. The renderer saved framebuffer
state after target allocation, so the forward pass could restore `GL_NONE` for
the default framebuffer draw buffer. On macOS core profile this makes the color
forward pass effectively disappear.

Fix:
- Save framebuffer, viewport, draw buffer and read buffer before shadow target allocation.
- Restore framebuffer, viewport, draw buffer and read buffer after shadow pass.
- Handle the empty-shadow-list path as well.

Affected examples included:
- `14_cache_dashed_texture_transform`
- `15_skinning_animation_test`
- `16_pbr_material_gallery`
- other shadow-enabled examples.
