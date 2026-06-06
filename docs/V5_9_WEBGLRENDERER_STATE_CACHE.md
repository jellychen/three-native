# v5.9 WebGLRenderer-style state/cache 深化

本版本继续沿 three.js WebGLRenderer 的内部架构方向收敛，重点不是新增视觉特性，而是把 renderer 的状态和缓存层拆清楚，便于后续真实模型、大场景、透明队列、后处理和 PMREM 继续稳定演进。

## 新增模块

### TextureUnitAllocator

位置：`src/renderer/cache/TextureUnitAllocator.hpp`

提供类似 three.js WebGLTextures 内部 texture unit 管理的能力：

- 查询 `GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS`
- 维护 texture id/version/target 到 texture unit 的稳定映射
- 避免重复 `glActiveTexture` / `glBindTexture`
- 支持 LRU 方式回收 texture unit
- 输出 requests / hits / misses / evictions

### WebGLStateCache

位置：`src/renderer/cache/WebGLStateCache.hpp`

面向 WebGLRenderer-style state cache：

- program binding cache
- framebuffer binding cache
- depthTest / depthWrite cache
- blend cache
- cullFace cache
- viewport / scissor cache
- redundant state call 统计

### RenderListPersistentCache

位置：`src/renderer/cache/RenderListPersistentCache.hpp`

用于缓存 scene/camera 签名对应的排序后 render list，后续可与 Object3D / Geometry / Material version 联动，减少大型静态场景每帧重复 project/sort 成本。

### RendererCacheDiagnostics

位置：`src/renderer/cache/RendererCacheDiagnostics.hpp`

统一输出：

- ProgramCache stats
- TextureUnitAllocator stats
- WebGLStateCache stats
- RenderListPersistentCache stats
- Geometry cache entries

`GLRenderer::diagnostics()` 已接入该结构。

## 新增测试

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 52_webglrenderer_state_cache_lab
```

测试内容：

- 100 个 mesh 共享 geometry
- 8 个材质轮换
- OrbitControls 动态相机
- 每秒输出 renderer cache diagnostics
- 观察 program cache、geometry cache、draw calls、triangles

## 后续可继续深化

v5.9 目前把状态/缓存对象和诊断入口补齐，并把 `GLRenderer::diagnostics()` 接上。下一步可以继续把现有 `GLState` / `GLResourceManager` 的具体 texture binding 和 uniform upload 逐步迁移到：

- `TextureUnitAllocator`
- `WebGLStateCache`
- `UniformCache`
- `RenderListPersistentCache`

避免一次性替换导致大面积渲染回归。
