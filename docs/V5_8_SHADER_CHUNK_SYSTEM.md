# v5.8 Shader Chunk System 重构

本版本开始把原来的单体 shader 拼接方式收敛为 three.js 风格的 shader chunk 体系。
目标不是一次性重写所有 shader，而是先建立稳定的 chunk registry、define 生成器、builder 和 chunk 边界，后续每个材质和 pass 都可以逐步迁移。

## 新增文件

```txt
src/shader/ShaderChunk.hpp
src/shader/ShaderChunk.cpp
examples/51_shader_chunk_system_lab/main.cpp
```

## Chunk Registry

当前注册的 chunk：

```txt
common
colorspace
tonemapping
packing
pbr_math
ibl
shadow
normal_perturb
physical
morph_vertex
skinning_vertex
instancing_vertex
project_vertex
lights_fragment
envmap_fragment
output_fragment
```

## ShaderChunk::defines(ProgramKey)

原先 `ShaderLib.cpp` 中分散的 `#define USE_*` 逻辑现在集中到：

```cpp
ShaderChunk::defines(const ProgramKey& key)
```

这一步的收益：

```txt
1. ProgramKey -> shader defines 的映射唯一化
2. 新增材质/贴图/光照特性时不再到处加宏
3. ProgramCache 的 hash 和 shader define 语义更容易对齐
4. 后续可以把 defines 输出到调试报告
```

## ShaderChunk::fragmentCore()

PBR/Physical/Lambert/Phong/Basic 共享的 fragment helper 现在通过：

```cpp
ShaderChunk::fragmentCore()
```

集中拼接：

```txt
common + colorspace + tonemapping + packing + pbr_math + physical + ibl + shadow + normal_perturb
```

`ShaderLib` 暂时仍保留原来的材质主体 shader，但 helper 已经 chunk 化。这样可以降低一次性重构风险。

## Chunk Boundary Comments

生成的 shader 中会插入：

```glsl
// <chunk:pbr_math>
...
// </chunk:pbr_math>
```

这能让 GL shader 编译错误更容易定位到具体 chunk。

## 新增测试

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 51_shader_chunk_system_lab
```

这个测试不会打开窗口，只会打印注册的 chunk 和生成的 Physical shader 统计信息。

## 后续迁移计划

v5.8 只是第一步。后续应该继续拆分：

```txt
1. vertex morph/skinning/instancing/project chunks
2. material parameter declaration chunks
3. lights_fragment direct light loop chunk
4. envmap_fragment IBL chunk
5. physical_fragment clearcoat/transmission/sheen chunks
6. shadowmap_fragment / shadowmap_pars chunks
7. postprocessing shader chunks
```

最终目标是接近 three.js：

```txt
ShaderLib + ShaderChunk + ProgramKey + ProgramCache
```

而不是一个越来越大的 `ShaderLib.cpp`。
