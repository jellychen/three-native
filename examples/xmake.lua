local examples = {
    "00_clear_screen",
    "01_basic_primitives",
    "03_pbr_lights",
    "04_fatline",
    "05_skinning",
    "06_physical_transmission",
    "07_transmission_render_target",
    "08_macos_glfw_native_test",
    "09_pbr_envmap",
    "10_pmrem_lut",
    "11_material_geometry_matrix",
    "12_shadow_map",
    "13_v2_0_threejs_stack",
    "14_cache_dashed_texture_transform",
    "15_skinning_animation_test",
    "16_pbr_material_gallery",
    "18_morph_targets_test",
    "20_postprocessing_stack",
    "21_v3_3_runtime_cache",
    "22_multi_light_shadow_test",
    "23_moving_lights_test",
    "24_light_falloff_spot_test",
    "27_shadow_completeness_lab",
    "26_physical_material_parity_lab",
    "25_standard_physical_pmrem_shadow_lab",
    "28_hdr_pmrem_pipeline",
    "29_pmrem_roughness_lod_lab",
    "31_standard_material_channel_lab",
    "32_renderer_architecture_lab",
    "33_pmrem_hdr_ibl",
    "34_standard_material_parity_lab",
    "35_light_system_parity_lab",
    "36_shadow_map_parity_lab",
    "37_physical_material_parity_lab",
    "39_animation_mixer_parity_lab",
    "40_morph_target_parity_lab",
    "41_line_points_helpers_parity_lab",
    "43_performance_cache_large_scene",
    "44_gpu_instancing_lab",
    "45_groups_multimaterial_drawrange",
    "46_transparent_transmissive_queue",
    "49_ktx2_transcoder_viewer",
    "50_pmrem_precision_lab",
    "51_shader_chunk_system_lab",
    "52_webglrenderer_state_cache_lab",
    "54_directional_shadow_only_lab",
    "55_spot_shadow_only_lab",
    "56_pbr_pmrem_specular_restore"
}

if has_config("use_angle") then
    table.insert(examples, "08_glfw_angle_renderer_test")
end

if has_config("enable_assimp") then
    table.insert(examples, "02_assimp_viewer")
    table.insert(examples, "17_assimp_multi_format_viewer")
    table.insert(examples, "19_gltf_validation_suite")
    table.insert(examples, "30_gltf_material_animation_validation")
    table.insert(examples, "38_asset_import_parity_lab")
    table.insert(examples, "47_regression_scene_runner")
    table.insert(examples, "48_gltf_extensions_texture_transform")
    table.insert(examples, "53_regression_stability_runner")
end

for _, name in ipairs(examples) do
    target(name)
        set_kind("binary")
        add_files(name .. "/main.cpp")
        add_deps("threecpp")
        add_includedirs("../src")
end
