// main.cpp
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "sceneBuilderClass.hpp"
#include "modelClass.hpp"

using std::string;
using std::vector;
using std::shared_ptr;
using std::make_shared;

int main(int argc, char** argv) {
    if (argc < 3) {
        return EXIT_FAILURE;
    }

    const string stlPath = argv[1];
    const long long numInstancesLL = std::atoll(argv[2]);
    if (numInstancesLL <= 0) {
        return EXIT_FAILURE;
    }
    const std::size_t numInstances = static_cast<std::size_t>(numInstancesLL);
    const string layout = (argc >= 4) ? string(argv[3]) : string("grid");

    sceneBuilderClass scene;

    shared_ptr<ModelObject> model = make_shared<ModelObject>(stlPath);

    scene.addObject(model);

    //see if needed
    scene.setModelBounds(glm::vec3(-0.5f), glm::vec3(0.5f)); // fallback; replace with real bounds if available

    vector<glm::mat4> mats = scene.makeInstanceTransforms(
        numInstances,
        layout,
        /*spacing*/ 2.5f,
        /*radius*/  25.0f,
        /*boxMin*/  glm::vec3(-20.0f),
        /*boxMax*/  glm::vec3( 20.0f)
    );

    // Upload instances to the scene (compute shader will cull them each frame)
    scene.setInstanceTransforms(mats);

    // Sensible camera defaults for this scene scale (aspect will update on resize)
    scene.setCamera(60.0f, 1280.0f/720.0f, 0.05f, 2000.0f);

    scene.run();

    return EXIT_SUCCESS;
}
