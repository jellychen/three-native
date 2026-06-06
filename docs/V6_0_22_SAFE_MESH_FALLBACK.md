# v6.0.22 Safe Mesh Forward Fallback

This version restores visible output for mesh-heavy examples on macOS while the full PBR/Physical shader path is being repaired.

The previous diagnostics showed:

- RenderList was populated.
- draw calls were executed.
- clip-space positions were visible.
- line/dashed programs rendered correctly.
- `THREECPP_FORCE_UNLIT=1` rendered mesh geometry.
- the full material programs did not render visible mesh output.

That isolates the bug to the full MeshStandard/MeshPhysical fragment path, not geometry, camera, VAO, viewport, culling or draw submission.

By default, MeshLambert/MeshPhong/MeshStandard/MeshPhysical triangle programs now use a robust MeshBasic-style fragment fallback while preserving UVs, color maps, alpha maps, vertex colors, groups, instancing and draw ranges.

To debug the experimental full PBR path, run:

```bash
THREECPP_ENABLE_EXPERIMENTAL_PBR=1 xmake run 14_cache_dashed_texture_transform
```

This is a stabilization patch, not the final PBR solution. The next step is to isolate and repair the full PBR/Physical shader path using the now-stable visible baseline.
