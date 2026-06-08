#pragma once
#include "core/object-3d.h"
#include "core/renderable.h"
#include "material/material.h"
#include <iostream>
#include <unordered_set>
#include <iomanip>

namespace THREE {

inline const char* color_space_name(ColorSpace cs) {
    return cs == ColorSpace::SRGB ? "sRGB" : "Linear";
}
inline const char* channel_name(TextureChannel c) {
    switch (c) { case TextureChannel::R: return "R"; case TextureChannel::G: return "G"; case TextureChannel::B: return "B"; case TextureChannel::A: return "A"; }
    return "?";
}
inline const char* side_name(Side s) {
    switch (s) { case Side::FrontSide: return "FrontSide"; case Side::BackSide: return "BackSide"; case Side::DoubleSide: return "DoubleSide"; }
    return "?";
}
inline const char* material_type_name(MaterialType t) {
    switch (t) {
        case MaterialType::MeshBasic: return "MeshBasic";
        case MaterialType::MeshLambert: return "MeshLambert";
        case MaterialType::MeshPhong: return "MeshPhong";
        case MaterialType::MeshStandard: return "MeshStandard";
        case MaterialType::MeshPhysical: return "MeshPhysical";
        case MaterialType::LineBasic: return "LineBasic";
        case MaterialType::LineDashed: return "LineDashed";
        case MaterialType::FatLine: return "FatLine";
        case MaterialType::Points: return "Points";
        case MaterialType::Sprite: return "Sprite";
        case MaterialType::Shader: return "Shader";
        case MaterialType::Depth: return "Depth";
        case MaterialType::Distance: return "Distance";
    }
    return "?";
}

inline void dump_texture_slot(std::ostream& out, const char* slot, const std::shared_ptr<Texture>& tex) {
    if (!tex) return;
    out << "    " << slot << ": " << (tex->sourcePath.empty() ? tex->name : tex->sourcePath)
        << " colorSpace=" << color_space_name(tex->colorSpace)
        << " uv=" << tex->uvChannel
        << " channel=" << channel_name(tex->scalarChannel)
        << " flipY=" << (tex->flipY ? "true" : "false")
        << " size=" << tex->width << "x" << tex->height;
    if (tex->embedded) out << " embedded";
    if (tex->compressed) out << " compressed=" << tex->compressionScheme;
    if (tex->hasTextureTransform) out << " transform(offset=" << tex->offset.x << "," << tex->offset.y << " repeat=" << tex->repeat.x << "," << tex->repeat.y << " rot=" << tex->rotation << ")";
    if (!tex->transcodeMessage.empty()) out << " note='" << tex->transcodeMessage << "'";
    out << "\n";
}

inline void dump_material(std::ostream& out, const Material& mat) {
    out << "  material id=" << mat.id << " name='" << mat.name << "' type=" << material_type_name(mat.type)
        << " opacity=" << mat.opacity
        << " transparent=" << (mat.transparent ? "true" : "false")
        << " alphaTest=" << mat.alphaTest
        << " depthWrite=" << (mat.depthWrite ? "true" : "false")
        << " side=" << side_name(mat.side) << "\n";
    if (auto* s = dynamic_cast<const MeshStandardMaterial*>(&mat)) {
        out << std::fixed << std::setprecision(3)
            << "    baseColor=(" << s->color.r << "," << s->color.g << "," << s->color.b << ")"
            << " roughness=" << s->roughness
            << " metalness=" << s->metalness
            << " emissive=(" << s->emissive.r << "," << s->emissive.g << "," << s->emissive.b << ")"
            << " emissiveIntensity=" << s->emissiveIntensity
            << " normalScale=(" << s->normalScale.x << "," << s->normalScale.y << ")"
            << " aoIntensity=" << s->aoMapIntensity
            << " channels(R/M/AO/Alpha)=" << channel_name(s->roughnessChannel) << "/" << channel_name(s->metalnessChannel) << "/" << channel_name(s->aoChannel) << "/" << channel_name(s->alphaChannel)
            << "\n";
        dump_texture_slot(out, "baseColor/map", s->map);
        dump_texture_slot(out, "normalMap", s->normalMap);
        dump_texture_slot(out, "roughnessMap", s->roughnessMap);
        dump_texture_slot(out, "metalnessMap", s->metalnessMap);
        dump_texture_slot(out, "aoMap", s->aoMap);
        dump_texture_slot(out, "emissiveMap", s->emissiveMap);
        dump_texture_slot(out, "alphaMap", s->alphaMap);
        if (auto* p = dynamic_cast<const MeshPhysicalMaterial*>(&mat)) {
            out << "    physical transmission=" << p->transmission
                << " thickness=" << p->thickness
                << " attenuationDistance=" << p->attenuationDistance
                << " ior=" << p->ior
                << " clearcoat=" << p->clearcoat
                << " sheen=" << p->sheen
                << " specularIntensity=" << p->specularIntensity
                << " iridescence=" << p->iridescence
                << " anisotropy=" << p->anisotropy << "\n";
            dump_texture_slot(out, "transmissionMap", p->transmissionMap);
            dump_texture_slot(out, "thicknessMap", p->thicknessMap);
            dump_texture_slot(out, "clearcoatMap", p->clearcoatMap);
            dump_texture_slot(out, "clearcoatRoughnessMap", p->clearcoatRoughnessMap);
            dump_texture_slot(out, "clearcoatNormalMap", p->clearcoatNormalMap);
            dump_texture_slot(out, "sheenColorMap", p->sheenColorMap);
            dump_texture_slot(out, "sheenRoughnessMap", p->sheenRoughnessMap);
        }
    }
}

inline void dump_scene_materials(Object3D& root, std::ostream& out = std::cout) {
    std::unordered_set<ObjectId> seen;
    root.traverse([&](Object3D& o) {
        if (auto* mesh = dynamic_cast<Mesh*>(&o)) {
            auto emit = [&](const std::shared_ptr<Material>& m) {
                if (!m || seen.count(m->id)) return;
                seen.insert(m->id);
                dump_material(out, *m);
            };
            emit(mesh->material);
            for (const auto& m : mesh->materials) emit(m);
        }
    });
    out << "Material dump complete. uniqueMaterials=" << seen.size() << "\n";
}

} // namespace THREE
