#include "regression/RegressionSceneRunner.hpp"
#include <iostream>
#include <fstream>

using namespace threecpp;

static void printUsage() {
    std::cerr << "Usage:\n"
              << "  47_regression_scene_runner <manifest.json>\n"
              << "  47_regression_scene_runner [--frames=N] [--size=1280x720] [--no-render] [--screenshots]\n"
              << "                             [--out=regression_out] [--report=report.json] [--markdown=report.md]\n"
              << "                             model.glb model.fbx ...\n\n"
              << "Manifest JSON subset:\n"
              << "{\n"
              << "  \"models\": [\"DamagedHelmet.glb\", \"Fox.glb\"],\n"
              << "  \"frames\": 60,\n"
              << "  \"width\": 1280,\n"
              << "  \"height\": 720,\n"
              << "  \"render\": true,\n"
              << "  \"screenshots\": true,\n"
              << "  \"outputDir\": \"regression_out\",\n"
              << "  \"reportJson\": \"regression_out/report.json\",\n"
              << "  \"reportMarkdown\": \"regression_out/report.md\",\n"
              << "  \"checkExternalFiles\": true,\n"
              << "  \"enableAnimation\": true,\n"
              << "  \"enableSkinning\": true,\n"
              << "  \"enableMorphTargets\": true,\n"
              << "  \"enableTextures\": true,\n"
              << "  \"enableShadows\": true\n"
              << "}\n";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printUsage();
        return 1;
    }

    RegressionSceneSpec spec;
    try {
        std::filesystem::path first(argv[1]);
        if (argc == 2 && first.extension() == ".json") {
            spec = RegressionManifestParser::parseFile(first);
        } else {
            spec = RegressionManifestParser::fromCommandLine(argc, argv, 1);
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse regression input: " << e.what() << "\n";
        return 2;
    }

    if (spec.models.empty()) {
        std::cerr << "No models specified.\n";
        printUsage();
        return 1;
    }

    if (spec.reportJson.empty()) spec.reportJson = spec.outputDir / "report.json";
    if (spec.reportMarkdown.empty()) spec.reportMarkdown = spec.outputDir / "report.md";

    std::cout << "threecpp v6.0 regression scene runner\n";
    std::cout << "models=" << spec.models.size()
              << " frames=" << spec.frames
              << " size=" << spec.width << "x" << spec.height
              << " render=" << (spec.render ? "true" : "false")
              << " screenshots=" << (spec.screenshots ? "true" : "false")
              << " animation=" << (spec.enableAnimation ? "true" : "false")
              << " out=" << spec.outputDir.string() << "\n";

    RegressionSceneRunner runner;
    RegressionRunReport report = runner.run(spec);
    std::cout << report.textSummary();
    std::cout << "JSON report: " << spec.reportJson.string() << "\n";
    std::cout << "Markdown report: " << spec.reportMarkdown.string() << "\n";

    return report.failCount == 0 ? 0 : 3;
}
