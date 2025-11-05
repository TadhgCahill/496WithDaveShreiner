#include "sceneBuilderClass.hpp"
#include "modelClass.hpp"

#include <iostream>
#include <cmath>
#include <algorithm>
#include <string>
#include <vector>
using namespace std;
//is the goal to add models live? like start with a render loop?

int main(int argc, char** argv) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <model_path> <num_instances>\n";
        return 1;
    }

    //user inputs name of stl then number of instances (maybe change)
    const string modelPath = argv[1];
    const int numInstancesInput = stoi(argv[2]);
    const int numInstances = max(1, numInstancesInput);

    sceneBuilderClass scene;//precurser to set global state (rn just sets window and view)

    shared_ptr<ModelObject> model;
    //keeps them from being spawned on top of each other
    try {
        model = make_shared<ModelObject>(modelPath);
    } catch (const exception& e) {
        cerr << "Failed to create ModelObject: " << e.what() << "\n";
        return 3;
    }

    vector<glm::mat4> instances;
    instances.reserve(numInstances);//sets siuze of vector from input

    const int grid = max(1, static_cast<int>(ceil(sqrt(static_cast<double>(numInstances)))));
    const float scale   = 1.0f;
    const float padding = 0.20f;

    // Base spacing from the model
    const float baseExtent = max(model->bboxSize().x, model->bboxSize().z);
    const float spacing = (baseExtent > 0.0f ? baseExtent : 1.0f) * scale * (1.0f + padding);

    for (int x = 0; x < grid && static_cast<int>(instances.size()) < numInstances; ++x) {
        for (int z = 0; z < grid && static_cast<int>(instances.size()) < numInstances; ++z) {
            glm::mat4 M(1.0f);
            const float tx = (x - grid / 2) * spacing;//stays
            const float tz = (z - grid / 2) * spacing; //stays
            M = glm::translate(M, glm::vec3(tx, 0.0f, tz));
            M = glm::scale(M, glm::vec3(scale));
            instances.push_back(M);
        }
    }

    // Apply instances and register the object with the scene.
    model->setInstanceTransforms(instances);
    scene.addObject(model);

    // Render loop.
    scene.run();
    return 0;
}
