#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>   // for glm::perspective, glm::lookAt
#include <glm/gtc/type_ptr.hpp>           // for glm::value_ptr if you need it


#include <iostream>
#include <vector>

// Simple GLSL shaders
const char* vShaderSrc = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* fShaderSrc = R"(
#version 330 core
in vec3 Normal;
out vec4 FragColor;

void main() {
    // simple directional light
    vec3 lightDir = normalize(vec3(0.5,1.0,0.3));
    float diff = max(dot(normalize(Normal), lightDir), 0.0);
    vec3 color = vec3(1.0, 0.6, 0.2) * diff + vec3(0.1); // orange + ambient
    FragColor = vec4(color, 1.0);
}
)";

// Utility: compile a shader and check for errors
GLuint compileShader(GLenum type, const char* src) {
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

int main() {
    // -------------------- OpenGL Context --------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "bunny", NULL, NULL);
    glfwMakeContextCurrent(window);
    glewInit();

    // -------------------- Load STL with Assimp --------------------
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        "Bunny-LowPoly.stl",
        aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_JoinIdenticalVertices //check these
    );
    if (!scene || !scene->HasMeshes()) {
        std::cerr << "Failed to load STL: " << importer.GetErrorString() << "\n";
        return -1;
    }

    aiMesh* mesh = scene->mMeshes[0]; // assume one mesh

    std::vector<float> vertices;
    vertices.reserve(mesh->mNumVertices * 6); // pos + normal
    for (unsigned i = 0; i < mesh->mNumVertices; ++i) {
        aiVector3D p = mesh->mVertices[i];
        aiVector3D n = mesh->mNormals[i];
        vertices.insert(vertices.end(), {p.x, p.y, p.z, n.x, n.y, n.z});
    }

    std::vector<unsigned int> indices;
    for (unsigned f = 0; f < mesh->mNumFaces; ++f) {
        const aiFace& face = mesh->mFaces[f];
        for (unsigned j = 0; j < face.mNumIndices; ++j)
            indices.push_back(face.mIndices[j]);
    }

    // -------------------- Upload to GPU --------------------
    GLuint VBO, VAO, EBO;
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

    // -------------------- Build Shader Program --------------------
    GLuint program = glCreateProgram();
    glAttachShader(program, compileShader(GL_VERTEX_SHADER, vShaderSrc));
    glAttachShader(program, compileShader(GL_FRAGMENT_SHADER, fShaderSrc));
    glLinkProgram(program);
    glDeleteShader(compileShader(GL_VERTEX_SHADER, vShaderSrc));
    glDeleteShader(compileShader(GL_FRAGMENT_SHADER, fShaderSrc));

    // basic transforms
    glm::mat4 projection = glm::perspective(glm::radians(30.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 view       = glm::lookAt(glm::vec3(0,0,3),
                            glm::vec3(0,0,0),
                            glm::vec3(0,1,0));

    glEnable(GL_DEPTH_TEST);

    //current version spins model (play around with more later)

    // -------------------- Main Loop --------------------
    while (!glfwWindowShouldClose(window))
    {
        // scale down and center the model
        // glm::mat4 model = glm::mat4(1.0f);
        // model = glm::scale(model, glm::vec3(0.01f));

        glfwPollEvents();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //will rotate cameraq but not model (eye)
        // const float radius = 10.0f;
        // float camX = sin(glfwGetTime()) * radius;
        // float camZ = cos(glfwGetTime()) * radius;
        // view = glm::lookAt(glm::vec3(camX, 0.0, camZ), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));

         // rotation angle in radians
        float t = (float)glfwGetTime();
        glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(0.01f));
        model = glm::rotate(model, t, glm::vec3(0.0f, 1.0f, 0.0f));
        
        glUseProgram(program);
        glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, &model[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, &projection[0][0]);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}
