#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>

void render(){
    glClear(GL_COLOR_BUFFER_BIT);
    
    glBegin(GL_TRIANGLES);
        glColor3f(1.0f, 0.0f, 0.0f); // Red color
        glVertex2f(0.0f, 0.0f); // Point at the center

        glColor3f(0.0f, 1.0f, 0.0f); // Green color
        glVertex2f(0.5f, 0.5f); // Point at (0.5, 0.5)

        glColor3f(0.0f, 0.0f, 1.0f); // white color?
        glVertex2f(-0.5f, 0.5f); // Point at (0.5, 0.5)
    glEnd();
}

int main() {
    if (!glfwInit()) return -1;

    GLFWwindow* window = glfwCreateWindow(800, 600, "color triangle", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glewInit();

    while (!glfwWindowShouldClose(window)) {
        render();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}