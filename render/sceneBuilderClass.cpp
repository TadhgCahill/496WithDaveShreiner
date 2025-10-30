#include "sceneBuilderClass.hpp"
using namespace std;

sceneBuilderClass::sceneBuilderClass(){
}

GLFWwindow* sceneBuilderClass::windowInit(int width, int height, std::string name){
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
    glfwMakeContextCurrent(window);

    glewInit();
    glEnable(GL_DEPTH_TEST);

    // Now it’s safe to touch OpenGL
    GLuint vs = compileShader(GL_VERTEX_SHADER, v.getVertexShader());
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, f.getFragShader());
    program = link(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);

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

GLuint sceneBuilderClass::compileShader(GLenum type, const char* src){
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024]; glGetShaderInfoLog(s, 1024, nullptr, log);
        cerr << "Shader compile error: " << log << "\n";
    }
    return s;
}

void sceneBuilderClass::setInstanceTransforms(const std::vector<glm::mat4>& t) {
    instanceMatrices = t;
    setupInstanceBuffer(); // (re)upload
}

void sceneBuilderClass::setupInstanceBuffer() {
    if (instanceMatrices.empty() || v.getVAO() == 0) return;

    if (!instanceVBO) glGenBuffers(1, &instanceVBO);
    glBindVertexArray(v.getVAO());

    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 instanceMatrices.size() * sizeof(glm::mat4),
                 instanceMatrices.data(),
                 GL_STATIC_DRAW);

    const GLsizei stride = sizeof(glm::mat4);
    for (int i = 0; i < 4; ++i) {
        glEnableVertexAttribArray(2 + i);
        glVertexAttribPointer(2 + i, 4, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(glm::vec4) * i));
        glVertexAttribDivisor(2 + i, 1);
    }

    glBindVertexArray(0);
}


GLuint sceneBuilderClass::link(GLuint vs, GLuint fs){
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok=0; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024]; glGetProgramInfoLog(p, 1024, nullptr, log);
        std::cerr << "Program link error: " << log << "\n";
    }
    return p;
}


void sceneBuilderClass::run() {  //test
    if (!window || !program) {
        std::cerr << "Scene not initialized (window/program).\n";
        return;
    }

    // Safety: make sure depth testing is on (in case it wasn't inited elsewhere)
    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Resize-aware viewport
        int w = 0, h = 0;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);

        // Clear (use a non-black color to confirm the loop is running)
        glClearColor(0.15f, 0.18f, 0.22f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Upload camera uniforms (view + projection) each frame
        uploadViewProjection();

        // Draw only if we actually have geometry & instances
        const bool haveGeometry =
            (v.getVAO() != 0) &&
            (v.getEBO() != 0) &&
            (!v.getIndices().empty());

        const bool haveInstances = !instanceMatrices.empty();

        if (haveGeometry && haveInstances) {
            glUseProgram(program);
            glBindVertexArray(v.getVAO());
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, v.getEBO());

            glDrawElementsInstanced(
                GL_TRIANGLES,
                static_cast<GLsizei>(v.getIndices().size()),
                GL_UNSIGNED_INT,
                nullptr,
                static_cast<GLsizei>(instanceMatrices.size())
            );

            glBindVertexArray(0);
        }
        // else: nothing to draw—window will just show the clear color.

        glfwSwapBuffers(window);
    }
}


void sceneBuilderClass::loadMesh(const std::string& path) {
    v.importer(path); // vertexClass::importer is void; it builds VAO/EBO & fills indices
}

void sceneBuilderClass::uploadViewProjection() {
    glUseProgram(program);

    float radius = 60.0f;    // was 10.0
    float camX = sin(glfwGetTime()) * radius;
    float camZ = cos(glfwGetTime()) * radius;
    view = glm::lookAt(glm::vec3(camX, 0.0f, camZ),
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 1.0f, 0.0f));


    GLint uV = glGetUniformLocation(program, "view");
    GLint uP = glGetUniformLocation(program, "projection");
    glUniformMatrix4fv(uV, 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(uP, 1, GL_FALSE, &projection[0][0]);
}
