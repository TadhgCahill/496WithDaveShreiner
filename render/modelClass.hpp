#include <GL/glew.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> 
#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp> 
#include <iostream>
using namespace std;

class vertexClass{
public:
    vertexClass();
    const char* getVertexShader(){return vertexShaderString;}
    void setVertexShader(const char* shader){vertexShaderString = shader;}
    void importer(string filename);
    void modelRotate(glm::mat4& model);
    GLuint getVAO(){return VAO;}
    GLuint getVBO(){return VBO;}
    GLuint getEBO(){return EBO;}
    std::vector<unsigned int> getIndices(){return indices;}

private:
    const char* vertexShaderString;
    GLuint VAO;
    GLuint EBO;
    GLuint VBO;
    vector<unsigned int> indices;
};

class fragmentClass{
public:
    fragmentClass();
    const char* getFragShader(){return fragShaderString;}
    void setFragShader(const char* shader){fragShaderString = shader;}
    //add some lighting functions

private:
    const char* fragShaderString;
};