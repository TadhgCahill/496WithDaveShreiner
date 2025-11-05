#include "sceneBuilderClass.hpp"
#include <iostream>
using namespace std;

//initialize window and camera
sceneBuilderClass::sceneBuilderClass() {
    windowInit(1280, 720, "Instanced Scene");
    setCamera(60.0f, 1280.0f / 720.0f, 0.1f, 1000.0f);
}

GLFWwindow* sceneBuilderClass::windowInit(int width, int height, string name) {
    if (!glfwInit()) {
        cerr << "GLFW init failed\n";
        return nullptr;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
    if (!window) {
        cerr << "GLFW window creation failed\n";
        return nullptr;
    }
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        cerr << "GLEW init failed\n";
        return nullptr;
    }
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
    return window;
}

void sceneBuilderClass::setCamera(float fovDeg, float aspect, float zNear, float zFar) {
    projection = glm::perspective(glm::radians(fovDeg), aspect, zNear, zFar);
}

void sceneBuilderClass::cameraRotate(glm::mat4& view){
    const float radius = 10.0f;
    float camX = sin(glfwGetTime()) * radius;
    float camZ = cos(glfwGetTime()) * radius;
    view = glm::lookAt(glm::vec3(camX, 0.0, camZ), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
}

//add generic model
void sceneBuilderClass::addObject(const shared_ptr<ModelObject>& obj) {
    objects_.push_back(obj);
    bindCameraPointers();
}

//should this be with models or is vector<glm::mat4>& fine?
void sceneBuilderClass::setInstanceTransforms(const vector<glm::mat4>& mats) {
    for (auto& o : objects_) {
        if (auto* m = dynamic_cast<ModelObject*>(o.get())) {
            m->setInstanceTransforms(mats);
        }
    }
}

void sceneBuilderClass::bindCameraPointers() {
    for (auto& o : objects_) {
        if (auto* m = dynamic_cast<ModelObject*>(o.get())) {
            m->bindCamera(&view, &projection);
        }
    }
}

void sceneBuilderClass::run() {
    if (!window) {
        cerr << "Call windowInit() first.\n";
        return;
    }
    // default camera if user forgot
    if (projection == glm::mat4(1.0f)) {
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        float aspect = (h > 0) ? float(w) / float(h) : 16.0f/9.0f;
        setCamera(60.0f, aspect, 0.1f, 1000.0f);
    }

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        bindCameraPointers();

        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        for (auto& obj : objects_) {
            obj->render();
        }

        glfwSwapBuffers(window);
    }
}
