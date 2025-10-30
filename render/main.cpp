// main.cpp
#include "sceneBuilderClass.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <filesystem>   // C++17

static void print_usage(const char* exe) {
    std::cerr << "Usage: " << exe << " <path-to-stl-or-obj> <num-instances>\n"
              << "Example: " << exe << " assets/bunny.obj 100\n";
}

int main(int argc, char** argv) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }

    const std::string path = argv[1];
    int numInstances = 0;
    try {
        numInstances = std::stoi(argv[2]);
    } catch (...) {
        std::cerr << "Invalid instance count. Using 1.\n";
        numInstances = 1;
    }
    if (numInstances <= 0) numInstances = 1;

    if (!std::filesystem::exists(path)) {
        std::cerr << "File not found: " << path << "\n";
        return 1;
    }

    sceneBuilderClass scene;

    // 1) Create window & GL context
    GLFWwindow* win = scene.windowInit(1280, 720, "Instanced Renderer");
    if (!win) return -1;

    // 2) Camera (fixed; adjust as you like)
    scene.setCamera(60.0f, 1280.0f / 720.0f, 0.1f, 500.0f);

    // 3) Load mesh into the scene’s internal vertex class
    //    (this should call vertexClass::importer(path) inside)
    scene.loadMesh(path);

    // 4) Build per-instance transforms: centered grid, each scaled to 0.1
    std::vector<glm::mat4> instances;
    instances.reserve(numInstances);

    const float uniformScale = 0.1f;                  // <- scale each instance to 0.1
    const int grid = static_cast<int>(std::ceil(std::sqrt(static_cast<double>(numInstances))));
    const float spacing = 20.0f;                       // increase if you still see overlap

    for (int z = 0; z < grid && static_cast<int>(instances.size()) < numInstances; ++z) {
        for (int x = 0; x < grid && static_cast<int>(instances.size()) < numInstances; ++x) {
            glm::mat4 M(1.0f);

            // Center the grid around the origin; spread by 'spacing'
            const float tx = (x - grid / 2) * spacing;
            const float tz = (z - grid / 2) * spacing;

            M = glm::translate(M, glm::vec3(tx, 0.0f, tz));
            M = glm::scale(M, glm::vec3(uniformScale));  // <- scale to 0.1

            instances.push_back(M);
        }
    }

    // 5) Upload instance matrices & render
    scene.setInstanceTransforms(instances);
    scene.run();
    return 0;
}
