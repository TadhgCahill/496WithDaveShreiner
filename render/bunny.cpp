#include "bunny.hpp"
using namespace std;

render::render(){
    //try simplifying both shaders
    vectorShaderString = R"(#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

out vec3 vNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    // transform position
    gl_Position = projection * view * model * vec4(aPos, 1.0);

    // normal should be transformed by the inverse-transpose of model (ignore non-uniform scale -> mat3)
    vNormal = mat3(transpose(inverse(model))) * aNormal;
}
)";


    fragShaderString = R"(#version 330 core
out vec4 FragColor;

void main()
{
    FragColor = vec4(1.0, 0.6, 0.2, 1.0); // orange
}
)";

}


const char* render::getVectorShader(){
    return vectorShaderString;
}

void render::setVectorShader(const char* shader){
    vectorShaderString = shader;
}

const char* render::getFragShader(){
    return fragShaderString;
}

void render::setFragShader(const char* shader){
    fragShaderString = shader;
}
//make sure this isnt missing any inxludes or globals
 GLuint render::compileShader(GLenum type, const char* src){
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512]; glGetShaderInfoLog(s, 512, NULL, log);
        std::cerr << "Shader compile error:\n" << log << "\n";
    }
    return s;
 }

void render::importer(string filename){// check includes needed // may only work for stl
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        filename,
        aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_JoinIdenticalVertices
    );
    // if (!scene || !scene->HasMeshes()) {
    //     std::cerr << "Failed to load STL: " << importer.GetErrorString() << "\n";
    //     return -1;
    // }

    aiMesh* mesh = scene->mMeshes[0]; // assume one mesh

    std::vector<float> vertices;
    vertices.reserve(mesh->mNumVertices * 6); // pos + normal
    for (unsigned i = 0; i < mesh->mNumVertices; ++i) {
        aiVector3D p = mesh->mVertices[i];
        aiVector3D n = mesh->mNormals[i];
        vertices.insert(vertices.end(), {p.x, p.y, p.z, n.x, n.y, n.z});
    }

    
    for (unsigned f = 0; f < mesh->mNumFaces; ++f) {
        const aiFace& face = mesh->mFaces[f];
        for (unsigned j = 0; j < face.mNumIndices; ++j)
            indices.push_back(face.mIndices[j]);
    }

    GLuint VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // layout: position (0), normal (1)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void render::modelRotate(glm::mat4& model) {
    float t = static_cast<float>(glfwGetTime());
    // rotate the incoming matrix (which you've already scaled in the loop)
    model = glm::rotate(model, t, glm::vec3(0.0f, 1.0f, 0.0f)); // t is radians/sec*time; fine for a spin
}

void render::cameraRotate(glm::mat4 view){
        const float radius = 10.0f;
        float camX = sin(glfwGetTime()) * radius;
        float camZ = cos(glfwGetTime()) * radius;
        view = glm::lookAt(glm::vec3(camX, 0.0, camZ), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
}

 GLFWwindow* render::windowInit(int width, int height, string name){
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "STL Loader", NULL, NULL);
    glfwMakeContextCurrent(window);
    glewInit();
    return window;
 }