#pragma once
#include "modelClass.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <memory>
#include <string>
using namespace std;

class sceneBuilderClass {
public:
    sceneBuilderClass();
    GLFWwindow* windowInit(int width, int height, string name);

    // camera
    void setCamera(float fovDeg, float aspect, float zNear, float zFar);
    void cameraRotate(glm::mat4& view);

    // objects
    void addObject(const shared_ptr<ModelObject>& obj);
    void setInstanceTransforms(const vector<glm::mat4>& mats);

    // main loop
    void run();

private:
    void bindCameraPointers();

private:
    GLFWwindow* window = nullptr;
    glm::mat4 view{1.0f}, projection{1.0f};
    vector<shared_ptr<ModelObject>> objects_;
};
