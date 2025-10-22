#include <GL/glew.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> 
#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp> 
#include <string>
#include <iostream>
#include <vector>
using namespace std;

class render{
public:
    render();
    const char* getVectorShader();
    void setVectorShader(const char* shader);
    const char* getFragShader();
    void setFragShader(const char* shader);
    GLuint compileShader(GLenum type, const char* src);
    void importer(string filename);
    void modelRotate(glm::mat4& model);
    void cameraRotate(glm::mat4 view);
    GLFWwindow* windowInit(int width, int height, string name);
    GLuint getVAO(){return VAO;}
    std::vector<unsigned int> getIndices(){return indices;}
    
private:
    const char* vectorShaderString;
    const char* fragShaderString;
    GLuint VAO;
    std::vector<unsigned int> indices;

};