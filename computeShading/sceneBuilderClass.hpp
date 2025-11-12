#pragma once
#include "modelClass.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <memory>
#include <string>
#include <glm/gtc/matrix_access.hpp>

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

    vector<glm::mat4> makeInstanceTransforms(size_t count, const string& layout, float spacing, float radius, const glm::vec3& boxMin, const glm::vec3& boxMax);
    void setModelBounds(const glm::vec3& minOS, const glm::vec3& maxOS);

private:
    void bindCameraPointers();

    // ==== Compute-culling helpers & GL resources ====
    void buildCullProgram_();
    void updateFrustumPlanes_(glm::vec4 planes[6]) const;

    // GL objects for culling
    GLuint cullProgram_ = 0;
    GLuint ssboMatrices_ = 0;    // input: per-instance world matrices (mat4)
    GLuint ssboVisible_  = 0;    // output: compacted visible indices (uint[])
    GLuint ssboCounter_  = 0;    // output: atomic counter (uint)

    // UBOs
    GLuint uboFrustum_   = 0;    // 6 planes
    GLuint uboAabb_      = 0;    // object-space AABB (min/max)

    // CPU-side cached data
    vector<glm::mat4> allInstances_;
    GLsizei maxInstances_ = 0;

    // STL object-space AABB
    glm::vec3 aabbMinOS_{-0.5f, -0.5f, -0.5f};
    glm::vec3 aabbMaxOS_{ 0.5f,  0.5f,  0.5f};
    bool      hasModelBounds_ = false;

    GLFWwindow* window = nullptr;
    glm::mat4 view{1.0f}, projection{1.0f};
    vector<shared_ptr<ModelObject>> objects_;
};
