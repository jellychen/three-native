# V6.0.19 Forward state guard

`THREECPP_FORCE_UNLIT=1` displaying geometry proved that matrices, VAO binding,
viewport and draw calls are valid. The remaining failure is in the normal forward
material path. This update reasserts critical OpenGL state before each forward draw
because shadow/transmission/post passes and older examples still use raw OpenGL
calls that can bypass the cached `GLState` object.

The guard reasserts:

- color mask
- depth range
- depth function
- depth test / depth write
- blend disable for opaque materials
- cull face / front face according to material side

This is intentionally conservative and favors correctness while the renderer is
being stabilized against the regression suite.
