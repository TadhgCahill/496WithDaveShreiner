#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cfloat>

#include <memory>
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
using namespace std;



class ModelObject{
public:

    ModelObject(const string& meshPath);
    ~ModelObject();

    void bindCamera(const glm::mat4* viewPtr, const glm::mat4* projPtr);
    void setInstanceTransforms(const vector<glm::mat4>& transforms);//here or in scene builder?

    //for spacing
    glm::vec3 bboxMin()  const { return bboxMin_; }
    glm::vec3 bboxMax()  const { return bboxMax_; }
    glm::vec3 bboxSize() const { return bboxMax_ - bboxMin_; }
    float     maxExtent() const { glm::vec3 s = bboxSize(); return max(s.x, max(s.y, s.z)); }

    void render();
    void initIndirect(GLsizei maxInstances);
    void setVisibleCount(GLuint visibleCount); // helper for run()

private:
    // shader utils
    static GLuint compile(GLenum type, const char* src);
    static GLuint link(GLuint vs, GLuint fs);

    // mesh utils
    void loadMesh(const string& path);
    void uploadMesh();
    void setupInstanceBuffer();

    //for spacing
    glm::vec3 bboxMin_{  FLT_MAX,  FLT_MAX,  FLT_MAX };
    glm::vec3 bboxMax_{ -FLT_MAX, -FLT_MAX, -FLT_MAX };

    // gpu
    GLuint vao_ = 0, vbo_ = 0, ebo_ = 0, instanceVbo_ = 0, program_ = 0;

    // cpu mesh
    vector<float> interleaved_;     // pos(3) + normal(3)
    vector<unsigned> indices_;

    // instancing
    vector<glm::mat4> instanceMats_;
    GLsizei instanceCount_ = 0;

    // camera (not owned)
    const glm::mat4* view_ = nullptr;
    const glm::mat4* proj_ = nullptr;

    // uniform locations
    GLint uView_ = -1;
    GLint uProj_ = -1;

    // defaults
    static const char* kDefaultVS;
    static const char* kDefaultFS;

    //for indirect
    struct DrawElementsIndirectCommand {
        GLuint count;          // number of indices per instance  (indices_.size())
        GLuint instanceCount;  // how many instances to draw      (visCount)
        GLuint firstIndex;     // 0
        GLuint baseVertex;     // 0
        GLuint baseInstance;   // 0
    };
    GLuint indirectBuffer_ = 0;
};
