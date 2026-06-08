local examples = {
    "00-clear-screen",
    "01-basic-primitives",
    "03-pbr-lights",
    "04-fatline",
    "05-skinning",
    "06-physical-transmission",
    "07-transmission-render-target",
    "08-macos-glfw-native-test",
    "09-pbr-envmap",
    "10-pmrem-lut",
    "11-material-geometry-matrix",
    "12-shadow-map",
    "13-v2-0-threejs-stack",
    "14-cache-dashed-texture-transform",
    "15-skinning-animation-test",
    "16-pbr-material-gallery",
    "18-morph-targets-test",
    "20-postprocessing-stack",
    "21-v3-3-runtime-cache",
    "22-multi-light-shadow-test",
    "23-moving-lights-test",
    "24-light-falloff-spot-test",
    "25-standard-physical-pmrem-shadow-lab",
    "26-physical-material-parity-lab",
    "27-shadow-completeness-lab",
    "28-hdr-pmrem-pipeline",
    "29-pmrem-roughness-lod-lab",
    "31-standard-material-channel-lab",
    "32-renderer-architecture-lab",
    "33-pmrem-hdr-ibl",
    "34-standard-material-parity-lab",
    "35-light-system-parity-lab",
    "36-shadow-map-parity-lab",
    "37-physical-material-parity-lab",
    "39-animation-mixer-parity-lab",
    "40-morph-target-parity-lab",
    "41-line-points-helpers-parity-lab",
    "43-performance-cache-large-scene",
    "44-gpu-instancing-lab",
    "45-groups-multimaterial-drawrange",
    "46-transparent-transmissive-queue",
    "49-ktx2-transcoder-viewer",
    "50-pmrem-precision-lab",
    "51-shader-chunk-system-lab",
    "52-webglrenderer-state-cache-lab",
    "54-directional-shadow-only-lab",
    "55-spot-shadow-only-lab",
    "56-pbr-pmrem-specular-restore",
    "57-furnace-test",
    "58-equirect-background",
    "59-helpers",
    "60-clipping",
    "61-rectarealight",
    "62-pbr-comprehensive"
}

if has_config("use_angle") then
    table.insert(examples, "08-glfw-angle-renderer-test")
end

if has_config("enable_assimp") then
    table.insert(examples, "02-assimp-viewer")
    table.insert(examples, "17-assimp-multi-format-viewer")
    table.insert(examples, "19-gltf-validation-suite")
    table.insert(examples, "30-gltf-material-animation-validation")
    table.insert(examples, "38-asset-import-parity-lab")
    table.insert(examples, "47-regression-scene-runner")
    table.insert(examples, "48-gltf-extensions-texture-transform")
    table.insert(examples, "53-regression-stability-runner")
end

for _, name in ipairs(examples) do
    target(name)
        set_kind("binary")
        add_files(name .. "/main.cc")
        add_deps("three")
        add_includedirs("../src")
end
