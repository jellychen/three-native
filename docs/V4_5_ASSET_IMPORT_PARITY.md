# v4.5 glTF / FBX / OBJ 导入对齐

这一版围绕 three.js `GLTFLoader` / 常用模型导入行为做收敛，重点不是增加新渲染效果，而是让真实模型加载后的材质、贴图、动画、骨骼和 morph 信息更可诊断、更接近 three.js 的数据语义。

## 新增

- `src/validation/ImportCompatibilityReport.hpp`
  - 统计节点、mesh、材质、贴图、骨骼、动画、morph target。
  - 诊断外部贴图是否丢失。
  - 检查 color texture / scalar texture 的 colorSpace 是否合理。
  - 标记 embedded texture、uv2、tangent、vertexColor、physical material 扩展使用情况。

- `examples/38_asset_import_parity_lab`
  - 加载 `glb / gltf / fbx / obj`。
  - 打印导入兼容性报告。
  - OrbitControls 观察模型。
  - 支持 SPACE 暂停/恢复动画，1/2/3 切换前三个 clip。
  - 默认启用 PBR 环境、方向光阴影、点光源、Grid/Axes helper。

## AssimpLoader 改进

- glTF `alphaMode=MASK` 默认 `alphaTest=0.5`，更接近 three.js / glTF 默认行为。
- glTF `occlusionTexture.strength` 映射到 `aoMapIntensity`。
- glTF normal texture scale 继续映射到 `normalScale`。
- `KHR_materials_emissive_strength` 映射到 `emissiveIntensity`。
- `KHR_materials_volume`：`thicknessFactor / attenuationColor / attenuationDistance`。
- `KHR_materials_ior`：`ior`。
- `KHR_materials_clearcoat`：`clearcoat / clearcoatRoughness`。
- `KHR_materials_sheen`：`sheenColor / sheenRoughness`。
- `KHR_materials_specular`：`specularColor / specularIntensity`。
- `KHR_materials_iridescence`：`iridescence / iridescenceIOR`。
- `KHR_materials_anisotropy`：`anisotropy`。
- texture wrap 增加 `MirroredRepeat` 映射。

## 运行

```bash
xmake f -m debug --use_angle=false --enable_assimp=true
xmake -r
xmake run 38_asset_import_parity_lab /path/to/model.glb
```

建议测试模型：

- `DamagedHelmet.glb`：PBR 基准。
- `BoomBox.glb`：metallicRoughness texture。
- `Lantern.glb`：alpha / emissive。
- `Fox.glb`：骨骼动画。
- `CesiumMan.glb`：骨骼动画。
- `RobotExpressive.glb`：骨骼 + morph target。
- `Mixamo FBX`：FBX 骨骼和单位/坐标验证。
- `OBJ + MTL`：传统材质兜底。

## 仍未完全对齐 three.js 的部分

- KTX2 / BasisU 解码尚未接入。
- Assimp 对 glTF KHR 扩展的 raw key 暴露随版本可能变化，后续要按真实模型继续补 key alias。
- FBX 坐标系、单位、骨骼层级仍需要用真实 Mixamo/Blender/3dsMax 导出模型压测。
- glTF `KHR_texture_transform` 目前保留了 UVTransform 链路，但 Assimp raw key 的覆盖还需要继续实测。
- glTF loader 的 bit-exact 行为仍不如 three.js `GLTFLoader`，v4.5 的定位是导入兼容性进一步收敛和诊断工具完善。
