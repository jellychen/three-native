# v6.0.34 PointLight Shadow Only Lab

This version changes `examples/36_shadow_map_parity_lab` into a point-light-only shadow test.

Changes:

- Removed AmbientLight, HemisphereLight, DirectionalLight, and SpotLight from the test scene.
- Kept only one animated `PointLight` with cubemap shadow.
- Lowered environment intensity so the point shadow is not washed out by IBL.
- The scene is intended for testing `THREECPP_DEBUG_PBR_POINT_SHADOW=1` and normal PBR point-shadow rendering.

Run:

```bash
xmake f -m debug --use_angle=false --enable_assimp=true
xmake -r
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_ENABLE_PBR_SHADOWS=1 xmake run 36_shadow_map_parity_lab
```

Debug point shadow factor:

```bash
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 THREECPP_ENABLE_PBR_SHADOWS=1 THREECPP_DEBUG_PBR_POINT_SHADOW=1 xmake run 36_shadow_map_parity_lab
```
