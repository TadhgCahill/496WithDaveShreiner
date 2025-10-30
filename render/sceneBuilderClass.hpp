#include "modelClass.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
using namespace  std;

class sceneBuilderClass{
public:
    sceneBuilderClass();
    GLFWwindow* windowInit(int width, int height, string name);
    void setCamera(float fovDeg, float aspect, float zNear, float zFar);
    void cameraRotate(glm::mat4& view);
    GLuint compileShader(GLenum type, const char* src);
    void setInstanceTransforms(const vector<glm::mat4>& t);
    void setupInstanceBuffer();
    GLuint link(GLuint vs, GLuint fs);
    void run();
    void loadMesh(const string& path);
    void uploadViewProjection();



private:
    glm::mat4 view{1.0f}, projection{1.0f};
    vector<glm::mat4> instanceMatrices;
    GLuint instanceVBO = 0;//check this
    vertexClass v;
    fragmentClass f;
    GLuint program = 0;
    GLFWwindow* window;
};