#include "bunny.hpp"

int main() {
    render bunny = render();
    // -------------------- OpenGL Context --------------------
    GLFWwindow* window = bunny.windowInit(800, 600, "STL Loader");

    // -------------------- Load STL with Assimp --------------------
    bunny.importer("Bunny-LowPoly.stl");

    // -------------------- Upload to GPU --------------------
    

    // -------------------- Build Shader Program --------------------
    
    GLuint vs = bunny.compileShader(GL_VERTEX_SHADER, bunny.getVectorShader());
    GLuint fs = bunny.compileShader(GL_FRAGMENT_SHADER, bunny.getFragShader());
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);

    // basic camera matrices
    GLint modelLoc = glGetUniformLocation(program, "model");
    GLint viewLoc  = glGetUniformLocation(program, "view");
    GLint projLoc  = glGetUniformLocation(program, "projection");

    // basic transforms
    float aspect = 800.0f/600.0f;
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    glm::mat4 view       = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0));
    glm::mat4 model      = glm::mat4(1.0f);

    glEnable(GL_DEPTH_TEST);

    glUseProgram(program);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);
    // -------------------- Main Loop --------------------
    while (!glfwWindowShouldClose(window))
    {
        // scale down and center the model
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(0.01f));
        bunny.modelRotate(model);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        glfwPollEvents();

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(bunny.getVAO());
        glDrawElements(GL_TRIANGLES, bunny.getIndices().size(), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}