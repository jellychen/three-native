# v4.3 ShadowMap 完整化

本版本把 shadow map 从 Directional/Spot 基础路径继续推进到 three.js 常用阴影语义：

- DirectionalLight shadow：正交 shadow camera、mapSize、bias、normalBias、radius。
- SpotLight shadow：透视 shadow camera、angle/distance 约束、PCF 采样。
- PointLight cubemap shadow：6 面 depth cubemap 渲染、距离比较采样、简单 cube PCF。
- SkinnedMesh / MorphTarget shadow：depth program 继续继承 skinning 和 morph define。
- 新增 `examples/36_shadow_map_parity_lab`，同时测试方向光、聚光灯和点光源阴影，并包含运动光源/运动投影物。

## 运行

```bash
xmake f -m debug --use_angle=false --enable_assimp=false
xmake -r
xmake run 36_shadow_map_parity_lab
```

## 说明

PointLight 阴影现在采用 cube depth texture。shader 里把 cubemap 深度反投影为线性距离并与 fragment 到点光源距离比较。它已经具备可用的 omnidirectional shadow 路径，但仍不是 three.js 的 bit-exact 实现；后续还要继续完善：

- PCFSoftShadowMap 的采样核和 radius 行为；
- VSMShadowMap；
- 透明物体阴影；
- 更严格的 normalBias 世界空间偏移；
- shadow camera/helper 可视化。
