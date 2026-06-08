#pragma once
#include "common.h"
#include "loader/assimp-loader.h"
#include "validation/gltf-validation-suite.h"
#include "validation/import-compatibility-report.h"
#include "performance/performance-cache.h"
#include "renderer/gl-renderer.h"
#include "platform/window.h"
#include "platform/gl-headers.h"
#include "core/scene.h"
#include "core/camera.h"
#include "controls/orbit-controls.h"
#include "helpers/helpers.h"
#include "light/light.h"
#include "ibl/environment.h"
#include "texture/texture-factory.h"
#include "animation/animation.h"
#include <chrono>
#include <regex>
#include <fstream>
#include <iomanip>
#include <map>

namespace THREE {

// v6.0 regression runner. This remains dependency-free: it consumes a small JSON
// subset instead of depending on nlohmann/json. The goal is to make real-model
// regression practical on developer machines while keeping the core library small.
struct RegressionSceneSpec {
    std::vector<std::filesystem::path> models;
    int frames = 60;
    int width = 1280;
    int height = 720;
    bool render = true;
    bool screenshots = false;
    bool checkExternalFiles = true;
    bool enableAnimation = true;
    bool enableSkinning = true;
    bool enableMorphTargets = true;
    bool enableTextures = true;
    bool enableShadows = true;
    bool continueOnError = true;
    std::filesystem::path outputDir = "regression_out";
    std::filesystem::path reportJson;
    std::filesystem::path reportMarkdown;
};

enum class RegressionStatus {
    Pass,
    Partial,
    Fail
};

inline const char* to_string(RegressionStatus status) {
    switch (status) {
        case RegressionStatus::Pass: return "PASS";
        case RegressionStatus::Partial: return "PARTIAL";
        case RegressionStatus::Fail: return "FAIL";
    }
    return "FAIL";
}

struct RegressionModelResult {
    std::filesystem::path path;
    RegressionStatus status = RegressionStatus::Fail;
    bool loaded = false;
    bool rendered = false;
    std::string error;
    std::string format;
    std::vector<std::string> warnings;
    std::vector<std::string> unsupportedExtensions;
    std::filesystem::path screenshotPath;

    GltfValidationStats validation;
    ImportCompatibilityReport compatibility;
    LargeSceneProfile sceneProfile;

    int drawCalls = 0;
    int instancedCalls = 0;
    int instances = 0;
    int triangles = 0;
    int lines = 0;
    int points = 0;
    int programs = 0;
    double loadMs = 0.0;
    double inspectMs = 0.0;
    double firstFrameMs = 0.0;
    double avgFrameMs = 0.0;
    std::uint64_t sceneSignature = 0;
};

struct RegressionRunReport {
    std::vector<RegressionModelResult> results;
    int passCount = 0;
    int partialCount = 0;
    int failCount = 0;

    void addResult(RegressionModelResult result) {
        switch (result.status) {
            case RegressionStatus::Pass: ++passCount; break;
            case RegressionStatus::Partial: ++partialCount; break;
            case RegressionStatus::Fail: ++failCount; break;
        }
        results.push_back(std::move(result));
    }

    std::string textSummary() const {
        std::ostringstream out;
        out << "Regression summary: pass=" << passCount
            << " partial=" << partialCount
            << " fail=" << failCount
            << " total=" << results.size() << "\n";
        for (const auto& r : results) {
            out << "[" << to_string(r.status) << "] " << r.path.string();
            if (!r.error.empty()) out << " error='" << r.error << "'";
            out << "\n";
            if (!r.loaded) continue;
            out << "       format=" << r.format
                << " loadMs=" << std::fixed << std::setprecision(2) << r.loadMs
                << " inspectMs=" << r.inspectMs
                << " firstFrameMs=" << r.firstFrameMs
                << " avgFrameMs=" << r.avgFrameMs
                << " calls=" << r.drawCalls
                << " instancedCalls=" << r.instancedCalls
                << " instances=" << r.instances
                << " tris=" << r.triangles
                << " programs=" << r.programs
                << " signature=" << r.sceneSignature << "\n"
                << "       " << GltfValidationSuite::summary(r.validation) << "\n"
                << "       " << r.compatibility.summary() << "\n";
            if (!r.screenshotPath.empty()) out << "       screenshot=" << r.screenshotPath.string() << "\n";
            for (const auto& warning : r.warnings) out << "       warning: " << warning << "\n";
            if (!r.compatibility.textureIssues.empty()) {
                out << "       textureIssues=" << r.compatibility.textureIssues.size() << "\n";
                for (const auto& issue : r.compatibility.textureIssues) {
                    out << "         - material='" << issue.material << "' slot=" << issue.slot
                        << " source='" << issue.source << "' " << issue.message << "\n";
                }
            }
        }
        return out.str();
    }
};

class RegressionReportWriter {
public:
    static std::string jsonEscape(const std::string& s) {
        std::ostringstream out;
        for (char c : s) {
            switch (c) {
                case '\\': out << "\\\\"; break;
                case '"': out << "\\\""; break;
                case '\n': out << "\\n"; break;
                case '\r': out << "\\r"; break;
                case '\t': out << "\\t"; break;
                default: out << c; break;
            }
        }
        return out.str();
    }

    static void writeJson(const RegressionRunReport& report, const std::filesystem::path& file) {
        if (file.empty()) return;
        std::filesystem::create_directories(file.parent_path().empty() ? std::filesystem::path(".") : file.parent_path());
        std::ofstream out(file);
        if (!out) throw std::runtime_error("Could not write regression JSON report: " + file.string());
        out << "{\n";
        out << "  \"summary\": {\"pass\": " << report.passCount
            << ", \"partial\": " << report.partialCount
            << ", \"fail\": " << report.failCount
            << ", \"total\": " << report.results.size() << "},\n";
        out << "  \"results\": [\n";
        for (std::size_t i = 0; i < report.results.size(); ++i) {
            const auto& r = report.results[i];
            out << "    {\n";
            out << "      \"path\": \"" << jsonEscape(r.path.string()) << "\",\n";
            out << "      \"status\": \"" << to_string(r.status) << "\",\n";
            out << "      \"loaded\": " << (r.loaded ? "true" : "false") << ",\n";
            out << "      \"rendered\": " << (r.rendered ? "true" : "false") << ",\n";
            out << "      \"format\": \"" << jsonEscape(r.format) << "\",\n";
            out << "      \"error\": \"" << jsonEscape(r.error) << "\",\n";
            out << "      \"screenshot\": \"" << jsonEscape(r.screenshotPath.string()) << "\",\n";
            out << "      \"timingsMs\": {\"load\": " << r.loadMs << ", \"inspect\": " << r.inspectMs
                << ", \"firstFrame\": " << r.firstFrameMs << ", \"avgFrame\": " << r.avgFrameMs << "},\n";
            out << "      \"renderer\": {\"drawCalls\": " << r.drawCalls
                << ", \"instancedCalls\": " << r.instancedCalls
                << ", \"instances\": " << r.instances
                << ", \"triangles\": " << r.triangles
                << ", \"lines\": " << r.lines
                << ", \"points\": " << r.points
                << ", \"programs\": " << r.programs << "},\n";
            out << "      \"validation\": {\"objects\": " << r.validation.objects
                << ", \"meshes\": " << r.validation.meshes
                << ", \"materials\": " << r.validation.materials
                << ", \"textures\": " << r.validation.textures
                << ", \"animations\": " << r.validation.animations
                << ", \"animationTracks\": " << r.validation.animationTracks
                << ", \"skeletons\": " << r.validation.skeletons
                << ", \"bones\": " << r.validation.bones
                << ", \"morphTargets\": " << r.validation.morphTargets << "},\n";
            out << "      \"compatibility\": {\"unresolvedExternalTextures\": " << r.compatibility.stats.unresolvedExternalTextures
                << ", \"textureIssues\": " << r.compatibility.textureIssues.size()
                << ", \"compressedTextures\": " << r.compatibility.stats.compressedTextures
                << ", \"ktx2Textures\": " << r.compatibility.stats.ktx2Textures
                << ", \"basisTextures\": " << r.compatibility.stats.basisTextures
                << ", \"textureTransforms\": " << r.compatibility.stats.textureTransforms << "},\n";
            out << "      \"warnings\": [";
            for (std::size_t w = 0; w < r.warnings.size(); ++w) {
                if (w) out << ", ";
                out << "\"" << jsonEscape(r.warnings[w]) << "\"";
            }
            out << "]\n";
            out << "    }" << (i + 1 == report.results.size() ? "\n" : ",\n");
        }
        out << "  ]\n";
        out << "}\n";
    }

    static void writeMarkdown(const RegressionRunReport& report, const std::filesystem::path& file) {
        if (file.empty()) return;
        std::filesystem::create_directories(file.parent_path().empty() ? std::filesystem::path(".") : file.parent_path());
        std::ofstream out(file);
        if (!out) throw std::runtime_error("Could not write regression Markdown report: " + file.string());
        out << "# threecpp v6.0 Regression Report\n\n";
        out << "Pass: **" << report.passCount << "**, Partial: **" << report.partialCount
            << "**, Fail: **" << report.failCount << "**, Total: **" << report.results.size() << "**\n\n";
        out << "| Status | Model | Format | Load ms | Avg frame ms | Draw calls | Triangles | Issues | Screenshot |\n";
        out << "|---|---|---:|---:|---:|---:|---:|---:|---|\n";
        for (const auto& r : report.results) {
            out << "| " << to_string(r.status)
                << " | `" << r.path.filename().string() << "`"
                << " | " << r.format
                << " | " << std::fixed << std::setprecision(2) << r.loadMs
                << " | " << r.avgFrameMs
                << " | " << r.drawCalls
                << " | " << r.triangles
                << " | " << (r.compatibility.textureIssues.size() + r.warnings.size() + (r.error.empty() ? 0 : 1))
                << " | " << (r.screenshotPath.empty() ? "" : r.screenshotPath.filename().string())
                << " |\n";
        }
        out << "\n## Details\n\n";
        for (const auto& r : report.results) {
            out << "### " << r.path.string() << "\n\n";
            out << "Status: **" << to_string(r.status) << "**\n\n";
            if (!r.error.empty()) out << "Error: `" << r.error << "`\n\n";
            out << "- " << GltfValidationSuite::summary(r.validation) << "\n";
            out << "- " << r.compatibility.summary() << "\n";
            for (const auto& w : r.warnings) out << "- Warning: " << w << "\n";
            for (const auto& issue : r.compatibility.textureIssues) {
                out << "- Texture issue: material=`" << issue.material << "`, slot=`" << issue.slot
                    << "`, source=`" << issue.source << "`, " << issue.message << "\n";
            }
            out << "\n";
        }
    }
};

class RegressionManifestParser {
public:
    static RegressionSceneSpec parseFile(const std::filesystem::path& file) {
        std::ifstream in(file);
        if (!in) throw std::runtime_error("Could not open regression manifest: " + file.string());
        std::stringstream buffer;
        buffer << in.rdbuf();
        RegressionSceneSpec spec = parseText(buffer.str());
        const auto base = file.parent_path();
        for (auto& p : spec.models) {
            if (p.is_relative()) p = base / p;
        }
        resolveRelative(spec.outputDir, base);
        resolveRelative(spec.reportJson, base);
        resolveRelative(spec.reportMarkdown, base);
        return spec;
    }

    static RegressionSceneSpec fromCommandLine(int argc, char** argv, int firstModelArg = 1) {
        RegressionSceneSpec spec;
        for (int i = firstModelArg; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--no-render") spec.render = false;
            else if (arg == "--screenshots") spec.screenshots = true;
            else if (arg == "--no-animation") spec.enableAnimation = false;
            else if (arg == "--no-texture-check") spec.checkExternalFiles = false;
            else if (arg == "--no-shadows") spec.enableShadows = false;
            else if (arg.rfind("--frames=", 0) == 0) spec.frames = std::max(1, std::stoi(arg.substr(9)));
            else if (arg.rfind("--size=", 0) == 0) {
                auto value = arg.substr(7);
                auto x = value.find('x');
                if (x != std::string::npos) {
                    spec.width = std::max(64, std::stoi(value.substr(0, x)));
                    spec.height = std::max(64, std::stoi(value.substr(x + 1)));
                }
            } else if (arg.rfind("--out=", 0) == 0) spec.outputDir = arg.substr(6);
            else if (arg.rfind("--report=", 0) == 0) spec.reportJson = arg.substr(9);
            else if (arg.rfind("--markdown=", 0) == 0) spec.reportMarkdown = arg.substr(11);
            else {
                spec.models.emplace_back(arg);
            }
        }
        return spec;
    }

private:
    static void resolveRelative(std::filesystem::path& p, const std::filesystem::path& base) {
        if (!p.empty() && p.is_relative()) p = base / p;
    }

    static RegressionSceneSpec parseText(const std::string& text) {
        RegressionSceneSpec spec;
        spec.frames = readInt(text, "frames", spec.frames);
        spec.width = readInt(text, "width", spec.width);
        spec.height = readInt(text, "height", spec.height);
        spec.render = readBool(text, "render", spec.render);
        spec.screenshots = readBool(text, "screenshots", spec.screenshots);
        spec.checkExternalFiles = readBool(text, "checkExternalFiles", spec.checkExternalFiles);
        spec.enableAnimation = readBool(text, "enableAnimation", spec.enableAnimation);
        spec.enableSkinning = readBool(text, "enableSkinning", spec.enableSkinning);
        spec.enableMorphTargets = readBool(text, "enableMorphTargets", spec.enableMorphTargets);
        spec.enableTextures = readBool(text, "enableTextures", spec.enableTextures);
        spec.enableShadows = readBool(text, "enableShadows", spec.enableShadows);
        spec.continueOnError = readBool(text, "continueOnError", spec.continueOnError);
        spec.outputDir = readString(text, "outputDir", spec.outputDir.string());
        spec.reportJson = readString(text, "reportJson", spec.reportJson.string());
        spec.reportMarkdown = readString(text, "reportMarkdown", spec.reportMarkdown.string());
        spec.models = readStringArray(text, "models");
        return spec;
    }

    static int readInt(const std::string& text, const std::string& key, int fallback) {
        std::regex re("\\\"" + key + "\\\"\\s*:\\s*(-?\\d+)");
        std::smatch m;
        return std::regex_search(text, m, re) ? std::stoi(m[1].str()) : fallback;
    }

    static bool readBool(const std::string& text, const std::string& key, bool fallback) {
        std::regex re("\\\"" + key + "\\\"\\s*:\\s*(true|false)");
        std::smatch m;
        return std::regex_search(text, m, re) ? (m[1].str() == "true") : fallback;
    }

    static std::filesystem::path readString(const std::string& text, const std::string& key, const std::string& fallback) {
        std::regex re("\\\"" + key + "\\\"\\s*:\\s*\\\"([^\\\"]*)\\\"");
        std::smatch m;
        return std::regex_search(text, m, re) ? std::filesystem::path(m[1].str()) : std::filesystem::path(fallback);
    }

    static std::vector<std::filesystem::path> readStringArray(const std::string& text, const std::string& key) {
        std::vector<std::filesystem::path> values;
        std::regex arrayRe("\\\"" + key + "\\\"\\s*:\\s*\\[([^\\]]*)\\]");
        std::smatch arrayMatch;
        if (!std::regex_search(text, arrayMatch, arrayRe)) return values;
        const std::string body = arrayMatch[1].str();
        std::regex itemRe("\\\"([^\\\"]+)\\\"");
        for (auto it = std::sregex_iterator(body.begin(), body.end(), itemRe); it != std::sregex_iterator(); ++it) {
            values.emplace_back((*it)[1].str());
        }
        return values;
    }
};

class RegressionSceneRunner {
public:
    RegressionRunReport run(const RegressionSceneSpec& spec) {
        RegressionRunReport report;
        std::filesystem::create_directories(spec.outputDir);
        for (const auto& model : spec.models) {
            RegressionModelResult result = runOne(spec, model);
            report.addResult(std::move(result));
            if (!spec.continueOnError && report.failCount > 0) break;
        }
        RegressionReportWriter::writeJson(report, spec.reportJson);
        RegressionReportWriter::writeMarkdown(report, spec.reportMarkdown);
        return report;
    }

private:
    using Clock = std::chrono::steady_clock;

    static double msSince(Clock::time_point start, Clock::time_point end) {
        return std::chrono::duration<double, std::milli>(end - start).count();
    }

    static void normalizeImportedScene(Object3D& root, bool shadows) {
        root.traverse([&](Object3D& object) {
            if (auto* mesh = dynamic_cast<Mesh*>(&object)) {
                mesh->castShadow = shadows;
                mesh->receiveShadow = shadows;
                if (mesh->material) mesh->material->markNeedsUpdate();
            }
        });
    }

    static std::string slug(const std::filesystem::path& p) {
        std::string s = p.stem().string();
        for (char& c : s) if (!std::isalnum(static_cast<unsigned char>(c))) c = '_';
        if (s.empty()) s = "model";
        return s;
    }

    static void writeFramebufferPPM(const std::filesystem::path& file, int width, int height) {
        std::filesystem::create_directories(file.parent_path().empty() ? std::filesystem::path(".") : file.parent_path());
        std::vector<unsigned char> pixels(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 3u);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadBuffer(GL_BACK);
        glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
        std::ofstream out(file, std::ios::binary);
        if (!out) throw std::runtime_error("Could not write screenshot: " + file.string());
        out << "P6\n" << width << " " << height << "\n255\n";
        for (int y = height - 1; y >= 0; --y) {
            out.write(reinterpret_cast<const char*>(pixels.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(width) * 3u),
                      static_cast<std::streamsize>(width * 3));
        }
    }

    static void classify(RegressionModelResult& result, const RegressionSceneSpec& spec) {
        if (!result.loaded || !result.error.empty()) {
            result.status = RegressionStatus::Fail;
            return;
        }
        if (spec.render && !result.rendered) {
            result.status = RegressionStatus::Fail;
            if (result.error.empty()) result.error = "render probe did not complete";
            return;
        }
        bool partial = false;
        if (!result.compatibility.textureIssues.empty()) partial = true;
        if (result.compatibility.stats.unresolvedExternalTextures > 0) partial = true;
        if (spec.enableAnimation && result.validation.animations > 0 && result.validation.animationTracks == 0) {
            result.warnings.push_back("animations reported but no animation tracks were imported");
            partial = true;
        }
        if (spec.enableSkinning && result.compatibility.stats.skinnedMeshes > 0 && result.compatibility.stats.bones == 0) {
            result.warnings.push_back("skinned meshes found but no bones were counted");
            partial = true;
        }
        if (result.compatibility.stats.compressedTextures > 0 && result.compatibility.stats.transcodeReadyTextures == 0 && result.compatibility.stats.rgbaFallbackTextures == 0) {
            result.warnings.push_back("compressed textures were detected but no uploaded/transcoded/fallback texture was reported");
            partial = true;
        }
        result.status = partial ? RegressionStatus::Partial : RegressionStatus::Pass;
    }

    RegressionModelResult runOne(const RegressionSceneSpec& spec, const std::filesystem::path& model) {
        RegressionModelResult result;
        result.path = model;
        AssimpLoaderOptions opts;
        opts.loadTextures = spec.enableTextures;
        opts.loadEmbeddedTextures = spec.enableTextures;
        opts.loadSkinning = spec.enableSkinning;
        opts.loadMorphTargets = spec.enableMorphTargets;
        opts.loadAnimations = spec.enableAnimation;
        opts.generateTangents = true;
        opts.validateData = true;

        AssimpLoadResult loaded;
        auto loadStart = Clock::now();
        try {
            AssimpLoader loader(opts);
            loaded = loader.loadResult(model);
            result.loadMs = msSince(loadStart, Clock::now());
        } catch (const std::exception& e) {
            result.error = e.what();
            classify(result, spec);
            return result;
        }
        if (!loaded.root) {
            result.error = "loader returned empty root";
            classify(result, spec);
            return result;
        }
        result.loaded = true;
        result.format = loaded.format;

        normalizeImportedScene(*loaded.root, spec.enableShadows);

        auto inspectStart = Clock::now();
        GltfValidationSuite validator;
        result.validation = validator.inspect(*loaded.root, loaded.animations);
        ImportCompatibilityInspector inspector;
        result.compatibility = inspector.inspect(*loaded.root, loaded.animations, spec.checkExternalFiles);
        result.sceneProfile = analyze_large_scene(*loaded.root);
        result.sceneSignature = scene_cache_signature(*loaded.root);
        result.inspectMs = msSince(inspectStart, Clock::now());

        if (spec.render) {
            try {
                renderProbe(spec, loaded, result);
            } catch (const std::exception& e) {
                result.error = e.what();
            }
        }
        classify(result, spec);
        return result;
    }

    void renderProbe(const RegressionSceneSpec& spec, AssimpLoadResult& loaded, RegressionModelResult& result) {
        Window window(spec.width, spec.height, "threecpp v6.0 regression runner");
        Scene scene;
        scene.backgroundColor = {0.016f, 0.018f, 0.024f};
        scene.environment = PMREMGenerator({256, 8, true}).fromEquirectangular(TextureFactory::makeEquirectangularGradient(1024, 512));
        scene.environmentIntensity = 1.25f;

        PerspectiveCamera camera(50.0f, window.aspect(), 0.02f, 4000.0f);
        camera.position = {4.0f, 2.5f, 7.0f};
        camera.lookAt({0.0f, 0.8f, 0.0f});
        OrbitControls controls(camera, window);
        controls.target = {0.0f, 0.8f, 0.0f};
        controls.setDistance(7.5f);

        auto ambient = make_ref<AmbientLight>();
        ambient->intensity = 0.55f;
        scene.add(ambient);
        auto hemi = make_ref<HemisphereLight>();
        hemi->intensity = 0.65f;
        hemi->skyColor = {0.50f, 0.62f, 0.84f};
        hemi->groundColor = {0.16f, 0.14f, 0.12f};
        scene.add(hemi);
        auto sun = make_ref<DirectionalLight>();
        sun->position = {5.0f, 8.0f, 5.0f};
        sun->intensity = 4.0f;
        sun->castShadow = spec.enableShadows;
        sun->shadow.enabled = spec.enableShadows;
        sun->shadow.mapSize = {1024, 1024};
        sun->shadow.bias = -0.00035f;
        sun->shadow.normalBias = 0.03f;
        sun->shadow.cameraLeft = -10.0f;
        sun->shadow.cameraRight = 10.0f;
        sun->shadow.cameraTop = 10.0f;
        sun->shadow.cameraBottom = -10.0f;
        sun->shadow.cameraNear = 0.1f;
        sun->shadow.cameraFar = 40.0f;
        scene.add(sun);
        scene.add(make_ref<GridHelper>(12.0f, 12));
        scene.add(loaded.root);

        AnimationMixer mixer(loaded.root.get());
        bool hasAnim = spec.enableAnimation && !loaded.animations.empty();
        if (hasAnim) {
            auto& action = mixer.clipAction(loaded.animations[0]);
            action.loop = LoopMode::Repeat;
            action.play();
        }

        GLRenderer renderer({spec.width, spec.height});
        renderer.setToneMapping(ToneMapping::ACESFilmic, 1.0f);
        renderer.setOutputColorSpace(ColorSpace::SRGB);

        double totalMs = 0.0;
        for (int i = 0; i < std::max(1, spec.frames) && !window.shouldClose(); ++i) {
            window.poll();
            if (window.keyPressed(GLFW_KEY_ESCAPE)) window.requestClose();
            if (hasAnim) mixer.update(1.0f / 60.0f);
            controls.update();
            auto frameStart = Clock::now();
            renderer.render(scene, camera);
            double frameMs = msSince(frameStart, Clock::now());
            if (i == 0) result.firstFrameMs = frameMs;
            totalMs += frameMs;
            if (spec.screenshots && i + 1 == std::max(1, spec.frames)) {
                result.screenshotPath = spec.outputDir / (slug(result.path) + ".ppm");
                writeFramebufferPPM(result.screenshotPath, spec.width, spec.height);
            }
            window.swapBuffers();
        }
        result.avgFrameMs = totalMs / static_cast<double>(std::max(1, spec.frames));
        result.drawCalls = renderer.info.calls;
        result.instancedCalls = renderer.info.instancedCalls;
        result.instances = renderer.info.instances;
        result.triangles = renderer.info.triangles;
        result.lines = renderer.info.lines;
        result.points = renderer.info.points;
        result.programs = renderer.info.programs;
        result.rendered = true;
    }
};

} // namespace THREE
