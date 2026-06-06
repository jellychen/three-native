# v6.0.35 PointLight Shadow Depth Compare Fix

This version fixes the PBR PointLight cubemap shadow compare.

The previous shader compared `length(lightToFragment) / far` against the depth
cubemap. However the cubemap stores hardware perspective depth from six 90°
shadow cameras, not linear radial depth. The correct receiver depth must be
computed from the dominant cube face distance:

```
faceDepth = max(abs(lightToFragment.x), abs(lightToFragment.y), abs(lightToFragment.z))
```

Then the face distance is converted to OpenGL perspective depth before comparing
with the sampled cubemap depth.

This removes the very thin/flickering point shadow and makes most occluders cast
visible shadows in the point-shadow-only lab.
