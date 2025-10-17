// main.cpp
// Build (Linux):
//   g++ -std=c++17 main.cpp -o triangle `pkg-config --cflags --libs glfw3` -lGL -ldl
//
// Build (macOS):
//   brew install glfw glm
//   clang++ -std=c++17 main.cpp -o triangle -I/opt/homebrew/include -L/opt/homebrew/lib \
//            -lglfw -framework OpenGL
//
// Build (Windows, MSYS2/MinGW):
//   pacman -S mingw-w64-ucrt-x86_64-glfw glm
//   g++ -std=c++17 main.cpp -o triangle -lglfw3 -lopengl32 -lgdi32

#define GLAD_GL_IMPLEMENTATION        // use GLAD2 single-header implementation in this file
#include <glad/glad.h>     // NOT <glad/gl.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <cmath>

// ---------- shader helpers ----------
static void checkCompile(GLuint obj, bool program, const char* label) {
    GLint ok = 0;
    if (program) {
        glGetProgramiv(obj, GL_LINK_STATUS, &ok);
        if (!ok) {
            GLint len = 0; glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &len);
            std::string log(len, '\0'); glGetProgramInfoLog(obj, len, nullptr, log.data());
            fprintf(stderr, "[LINK ERR] %s:\n%s\n", label, log.c_str());
            std::exit(EXIT_FAILURE);
        }
    } else {
        glGetShaderiv(obj, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            GLint len = 0; glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &len);
            std::string log(len, '\0'); glGetShaderInfoLog(obj, len, nullptr, log.data());
            fprintf(stderr, "[COMPILE ERR] %s:\n%s\n", label, log.c_str());
            std::exit(EXIT_FAILURE);
        }
    }
}

static GLuint makeShader(GLenum type, const char* src, const char* label) {
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);
    checkCompile(sh, false, label);
    return sh;
}

static GLuint makeProgram(const char* vs, const char* fs) {
    GLuint v = makeShader(GL_VERTEX_SHADER,   vs, "vertex");
    GLuint f = makeShader(GL_FRAGMENT_SHADER, fs, "fragment");
    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    checkCompile(p, true, "program");
    glDeleteShader(v);
    glDeleteShader(f);
    return p;
}

// ---------- shaders ----------
static const char* kVertexSrc = R"(#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

void main() {
    gl_Position = uProj * uView * uModel * vec4(aPos, 1.0);
}
)";

static const char* kFragmentSrc = R"(#version 330 core
out vec4 FragColor;
void main() {
    FragColor = vec4(0.2, 0.8, 0.2, 1.0); // green
}
)";

// ---------- window resize ----------
static void framebuffer_size_callback(GLFWwindow*, int w, int h) {
    glViewport(0, 0, w, h);
}

int main() {
    if (!glfwInit()) {
        fprintf(stderr, "Failed to init GLFW\n");
        return EXIT_FAILURE;
    }

    // Request an OpenGL 3.3 Core profile context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(800, 600, "Growing Triangle", nullptr, nullptr);
    if (!window) {
        fprintf(stderr, "Failed to create window\n");
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSwapInterval(1); // vsync

    // if (!gladLoadGL(glfwGetProcAddress)) {
    //     fprintf(stderr, "Failed to load GL via GLAD\n");
    //     return EXIT_FAILURE;
    // }

    // Triangle geometry in NDC (so identity view/proj is fine)
    float vertices[] = {
        // x, y, z
         0.0f,  0.5f, 0.0f,  // top
        -0.5f, -0.5f, 0.0f,  // bottom-left
         0.5f, -0.5f, 0.0f   // bottom-right
    };

    GLuint vao=0, vbo=0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Shaders & program
    GLuint prog = makeProgram(kVertexSrc, kFragmentSrc);
    glUseProgram(prog);

    // Uniform locations
    GLint locModel = glGetUniformLocation(prog, "uModel");
    GLint locView  = glGetUniformLocation(prog, "uView");
    GLint locProj  = glGetUniformLocation(prog, "uProj");

    // Static view/projection (identity — triangle already in NDC)
    glm::mat4 V(1.0f);
    glm::mat4 P(1.0f);
    glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(V));
    glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(P));

    // Render loop
    while (!glfwWindowShouldClose(window)) {
        // animate scale: oscillate between 0.5 and 1.5
        float t = static_cast<float>(glfwGetTime());
        float s = 1.0f + 0.5f * std::sin(t * 2.0f); // speed factor 2.0

        glm::mat4 M(1.0f);
        M = glm::scale(M, glm::vec3(s, s, s));
        glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(M));

        // clear and draw
        glClearColor(0.07f, 0.07f, 0.09f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(prog);

    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
