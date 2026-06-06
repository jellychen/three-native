# V6.0.44 Light falloff + dashed line shader fix

Fixes:

- `41_line_points_helpers_parity_lab` shader compile failure caused by `USE_DASHED_LINE` code using `dashScale`, `dashSize`, `gapSize` in shader permutations that did not declare the uniforms. The uniforms are now declared once in the common fragment header under `USE_DASHED_LINE`.
- `24_light_falloff_spot_test` now makes SpotLight cone changes visible by reducing fill/point-light interference, widening the animated cone range, increasing SpotLight intensity, and adding a live `SpotLightHelper`.

Run:

```bash
xmake f -m debug --use_angle=false --enable_assimp=true
xmake -r
xmake run 24_light_falloff_spot_test
xmake run 41_line_points_helpers_parity_lab
```
