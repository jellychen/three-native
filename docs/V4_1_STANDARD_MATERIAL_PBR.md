# v4.1 MeshStandardMaterial / PBR 对齐

这一版在 v4.0 PMREM / HDR IBL 的基础上，集中推进 `MeshStandardMaterial` 的 three.js 行为对齐。

## 核心改动

### 1. Standard 材质 API 增强

- `Material::flatShading`
- `Material::premultipliedAlpha`
- `Material::toneMapped`
- `MeshStandardMaterial::bumpScale`
- `MeshStandardMaterial::roughnessChannel`
- `MeshStandardMaterial::metalnessChannel`
- `MeshStandardMaterial::aoChannel`
- `MeshStandardMaterial::alphaChannel`

默认通道继续对齐 glTF / three.js：

- roughness: G
- metalness: B
- ao: R
- alpha: G

### 2. Shader 对齐

新增 shader define：

- `USE_FLAT_SHADING`
- `USE_PREMULTIPLIED_ALPHA`
- `USE_BUMPMAP`

改进：

- `normalMap` 使用 dFdx/dFdy TBN fallback。
- `bumpMap` 走 derivative bump perturbation。
- `flatShading` 在 fragment 阶段基于 world position derivative 重建面法线。
- `alphaMap / roughnessMap / metalnessMap / aoMap` 使用可配置 scalar channel。
- `displacementMap` 顶点阶段使用 `uvTransform` 后的 UV。
- 输出阶段统一走 `finalOutput()`，支持 tone mapping、output color space 和 premultiplied alpha。

### 3. Renderer 输出行为

新增：

```cpp
renderer.setOutputColorSpace(ColorSpace::SRGB);
```

并向 shader 上传：

- `toneMappingMode`
- `outputColorSpace`
- `premultipliedAlpha`

### 4. GLState 对齐

- `colorWrite`
- `polygonOffset`
- native OpenGL 下 `wireframe -> glPolygonMode`
- premultiplied alpha blending: `ONE, ONE_MINUS_SRC_ALPHA`

ANGLE / OpenGL ES 路径下保留 wireframe 兼容 no-op。

## 新增测试

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 34_standard_material_parity_lab
```

测试内容：

- metallicRoughnessTexture G/B 通道
- aoMap uv2
- normalMap + normalScale
- bumpMap + bumpScale
- flatShading
- wireframe
- premultiplied transparent material
- ACES tone mapping + SRGB output

## 下一步

继续计划：

- v4.2 光源系统对齐
- v4.3 ShadowMap 完整化
- v4.4 MeshPhysicalMaterial
