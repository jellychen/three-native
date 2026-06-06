# v6.0.17 Force-visible diagnostics

This build adds a hard diagnostic path for the "background only but drawCalls > 0" failure.

New environment switches:

```bash
THREECPP_FORCE_UNLIT=1
```

Forces forward rendering through a minimal unlit MeshBasic shader, disables depth test, culling and blending, and keeps color writes on. This isolates PBR/shadow/texture/depth/cull issues from geometry/matrix/VAO issues.

```bash
THREECPP_DEBUG_DRAW=1
```

Prints every draw item: object name, material type, program id, VAO id, draw mode, group start/count, and the clip-space origin.

```bash
THREECPP_DEBUG_GL_VERBOSE=1
```

Prints GL_NO_ERROR after each draw. Without it only GL errors are printed.

Suggested command:

```bash
THREECPP_FORCE_UNLIT=1 THREECPP_DEBUG_DRAW=1 xmake run 14_cache_dashed_texture_transform
```

If objects become visible with FORCE_UNLIT, the remaining bug is in PBR/material/shadow/depth/culling state.
If objects are still invisible and GL errors are clean, inspect the printed clipOrigin values. If clipOrigin.w is negative or the normalized clip coordinates are outside [-1, 1], the issue is camera/object matrix math. If clipOrigin looks valid, the issue is VAO/attribute binding or framebuffer state.
