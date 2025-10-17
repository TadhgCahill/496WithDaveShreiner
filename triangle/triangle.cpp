#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>

// Vertex shader source (GLSL)
const char* vertexShaderSource = R"glsl(
#version 330 core
layout(location = 0) in vec3 aPos;

void main()
{
    gl_Position = vec4(aPos, 1.0);
}
)glsl";

// Fragment shader source (GLSL)
const char* fragmentShaderSource = R"glsl(
#version 330 core
out vec4 FragColor;

void main()
{
    FragColor = vec4(1.0, 0.5, 0.2, 1.0);  // Orange color
}
)glsl";

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main()
{
    // 1. Initialize GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }
    // Set version to 3.3, core profile (modern OpenGL)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 2. Create a window
    GLFWwindow* window = glfwCreateWindow(800, 600, "First Triangle", NULL, NULL);
    if (window == NULL)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // 3. Initialize GLEW (after context creation)
    glewExperimental = GL_TRUE;  // needed for some newer functionalities
    GLenum glewInitResult = glewInit();
    if (GLEW_OK != glewInitResult)
    {
        std::cerr << "Failed to initialize GLEW: "
                  << glewGetErrorString(glewInitResult) << "\n";
        glfwTerminate();
        return -1;
    }

    // 4. Build and compile shader programs

    // Vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    {
        GLint success;
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char infoLog[512];
            glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
            std::cerr << "ERROR: Vertex shader compilation failed\n" 
                      << infoLog << "\n";
        }
    }

    // Fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    {
        GLint success;
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            char infoLog[512];
            glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
            std::cerr << "ERROR: Fragment shader compilation failed\n"
                      << infoLog << "\n";
        }
    }

    // Shader program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    {
        GLint success;
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (!success)
        {
            char infoLog[512];
            glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
            std::cerr << "ERROR: Shader program linking failed\n"
                      << infoLog << "\n";
        }
    }
    // Clean up individual shader objects once linked
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // 5. Set up vertex data, buffers, and configure vertex attributes

    float vertices[] = {
         // positions
         0.0f,  0.5f, 0.0f,   // top
        -0.5f, -0.5f, 0.0f,   // bottom left
         0.5f, -0.5f, 0.0f    // bottom right
    };

    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // bind the Vertex Array Object first
    glBindVertexArray(VAO);

    // then bind and set vertex buffer(s)
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // then configure vertex attribute(s)
    glVertexAttribPointer(0,                     // attribute 0, matches layout location in vertex shader
                          3,                     // size = 3 floats per vertex (x, y, z)
                          GL_FLOAT,              // type
                          GL_FALSE,              // normalized?
                          3 * sizeof(float),     // stride: space between consecutive vertex attributes
                          (void*)0);             // offset in buffer
    glEnableVertexAttribArray(0);

    // Unbind to clean up (optional)
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // 6. Render loop
    while (!glfwWindowShouldClose(window))
    {
        // input
        processInput(window);

        // rendering commands
        // glClearColor(0.2f, 0.3f, 0.3f, 1.0f);  // background color
        glClear(GL_COLOR_BUFFER_BIT);

        // draw triangle
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);  // draw 3 vertices starting at index 0

        // swap buffers and poll IO events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 7. Clean up
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    // terminate GLFW
    glfwTerminate();
    return 0;
}
