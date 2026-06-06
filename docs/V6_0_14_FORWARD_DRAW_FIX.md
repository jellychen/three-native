# v6.0.14 Forward draw / queue fix

Fixes examples showing only the blue/background clear after example 14.

Changes:

- Default `Blending::Normal` no longer classifies a material as transparent.
- Alpha-tested opaque materials remain in the opaque queue.
- After shadow rendering, GLRenderer now forces default framebuffer, draw/read buffers, color mask, depth mask, polygon offset and cached GL state back to a known forward-render state.
- This addresses macOS Core OpenGL state leakage where background clear was visible but meshes were not drawn.
