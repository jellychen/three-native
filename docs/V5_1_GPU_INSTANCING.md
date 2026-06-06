# v5.1 GPU Instancing

This version connects the existing `InstancedMesh` data model to a real GPU instancing render path.

## Implemented

- `InstancedMesh::instanceMatrices`
- `InstancedMesh::instanceColors`
- versioned dynamic upload for instance matrix/color buffers
- vertex attributes:
  - location 11-14: `instanceMatrix` columns
  - location 6: `instanceColor`
- `glVertexAttribDivisor(..., 1)`
- `glDrawElementsInstanced` / `glDrawArraysInstanced`
- instanced forward render path
- instanced shadow depth path
- `ProgramKey::useInstancing`
- `ProgramKey::useInstanceColor`
- renderer stats:
  - `renderer.info.instancedCalls`
  - `renderer.info.instances`

## Attribute layout note

The instance matrix uses locations 11-14 to stay under the portable WebGL2/OpenGL ES 3 minimum of 16 vertex attributes. Instance color uses location 6, which is otherwise used by line distance and not by mesh rendering.

Current limitation: instancing and morph normals share attribute locations. For portability, the renderer disables morph-normal attributes when instancing is enabled. Position morph targets still use locations 7-10.

## Test

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 44_gpu_instancing_lab
```

The test renders 10,000 sphere instances with per-instance transforms and colors, plus dynamic updates to a subset of instance matrices each frame.

Expected console output includes:

```txt
calls=...
instancedCalls=1+
instances=10000+
```

Shadow rendering may add additional instanced calls depending on enabled lights.
