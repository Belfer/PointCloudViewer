#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <chrono>
#include <thread>

#include <glad/glad.h>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include "imgui_impl.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define WIN_TITLE "PCLViewer"
#define WIN_WIDTH 640
#define WIN_HEIGHT 480
#define GL_MAJOR 3
#define GL_MINOR 3

static const char *shape_vert =
"#version 330 core\n\
    layout(location = 0) in vec3 POSITION;\n\
    uniform mat4 MVP;\n\
    void main() {\n\
        gl_Position = vec4(POSITION, 1.0) * MVP;\n\
    }\n";

static const char *shape_frag =
"#version 330 core\n\
    out vec4 frag;\n\
    uniform vec4 Color;\n\
    void main() {\n\
        frag = Color;\n\
    }\n";

static const char *pointcloud_vert =
    "#version 330 core\n\
    layout(location = 0) in vec3 POSITION;\n\
    layout(location = 1) in vec3 NORMAL;\n\
    out vec3 _Normal;\n\
    uniform mat4 MVP;\n\
    void main() {\n\
        gl_Position = vec4(POSITION, 1.0) * MVP;\n\
        _Normal = NORMAL;\n\
    }\n";

static const char *pointcloud_frag =
    "#version 330 core\n\
    in vec3 _Normal;\n\
    out vec4 frag;\n\
    uniform vec3 LightDir;\n\
    uniform vec4 LightCol;\n\
    uniform vec4 AmbientCol;\n\
    void main() {\n\
        float d = dot(_Normal, -LightDir);\n\
        frag = AmbientCol + d * LightCol;\n\
    }\n";

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

static bool createShader(GLuint &prog, const char *vertSrc, const char *fragSrc)
{
    GLint success = 0;
    GLchar errBuff[1024] = { 0 };

    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &vertSrc, NULL);
    glCompileShader(vert);

    glGetShaderiv(vert, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        glGetShaderInfoLog(vert, sizeof(errBuff), NULL, errBuff);
        std::cerr << errBuff << std::endl;
        return false;
    }

    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &fragSrc, NULL);
    glCompileShader(frag);

    glGetShaderiv(frag, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        glGetShaderInfoLog(frag, sizeof(errBuff), NULL, errBuff);
        std::cerr << errBuff << std::endl;
        return false;
    }

    prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);

    glLinkProgram(prog);

    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        glGetShaderInfoLog(frag, sizeof(errBuff), NULL, errBuff);
        std::cerr << errBuff << std::endl;
        return false;
    }

    glValidateProgram(prog);

    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        glGetShaderInfoLog(frag, sizeof(errBuff), NULL, errBuff);
        std::cerr << errBuff << std::endl;
        return false;
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
    return true;
}

int main(int argc, char ** args)
{
    if (argc < 2) {
        std::cerr << "Please provide path to obj!\n";
        exit(EXIT_FAILURE);
    }

    GLFWwindow* window;
    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
        exit(EXIT_FAILURE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, GL_MAJOR);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, GL_MINOR);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, WIN_TITLE, NULL, NULL);
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    glfwSetKeyCallback(window, key_callback);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval(1);

    // Setup ImGui binding
    ImGui_ImplGlfwGL3_Init(window, true);

    printf("%s\n", glGetString(GL_VERSION));
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_DEPTH_CLAMP);
    glEnable(GL_MULTISAMPLE);
    glDisable(GL_CULL_FACE);

    std::string inputfile = args[1];
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string err;
    bool ret = tinyobj::LoadObj(shapes, materials, err, inputfile.c_str());

    if (!err.empty())
        std::cerr << err << std::endl;

    if (!ret)
        exit(1);

    std::vector<std::pair<GLuint, size_t>> meshes;

    glm::vec3 min(0), max(0), tmp(0);
    for (size_t i = 0; i < shapes.size(); i++) {
        size_t posCount = shapes[i].mesh.positions.size();
        size_t norCount = shapes[i].mesh.normals.size();
        GLuint mesh = 0;
        GLuint posVBO = 0;
        GLuint norVBO = 0;

        for (size_t j = 0; j < shapes[i].mesh.positions.size();) {
            tmp.x = shapes[i].mesh.positions[j++];
            tmp.y = shapes[i].mesh.positions[j++];
            tmp.z = shapes[i].mesh.positions[j++];
            min.x = tmp.x < min.x ? tmp.x : min.x;
            min.y = tmp.y < min.y ? tmp.y : min.y;
            min.z = tmp.z < min.z ? tmp.z : min.z;
            max.x = tmp.x > max.x ? tmp.x : max.x;
            max.y = tmp.y > max.y ? tmp.y : max.y;
            max.z = tmp.z > max.z ? tmp.z : max.z;
        }

        glGenVertexArrays(1, &mesh);
        glBindVertexArray(mesh);

        glGenBuffers(1, &posVBO);
        glBindBuffer(GL_ARRAY_BUFFER, posVBO);
        glBufferData(GL_ARRAY_BUFFER, posCount * sizeof(float), &shapes[i].mesh.positions[0], GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glGenBuffers(1, &norVBO);
        glBindBuffer(GL_ARRAY_BUFFER, norVBO);
        glBufferData(GL_ARRAY_BUFFER, norCount * sizeof(float), &shapes[i].mesh.normals[0], GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindVertexArray(0);
        glDeleteBuffers(1, &posVBO);
        meshes.emplace_back(std::make_pair(mesh, posCount));
    }

    shapes.clear();
    materials.clear();

    float boundsData[] = {
        min.x, min.y, min.z,
        max.x, min.y, min.z,
        min.x, max.y, min.z,
        max.x, max.y, min.z,
        min.x, min.y, max.z,
        max.x, min.y, max.z,
        min.x, max.y, max.z,
        max.x, max.y, max.z
    };

    GLuint boundsIndices[] = {
        1, 2, 3
    };

    GLuint bounds;
    glGenVertexArrays(1, &bounds);
    glBindVertexArray(bounds);

    GLuint boundsVBO;
    glGenBuffers(1, &boundsVBO);
    glBindBuffer(GL_ARRAY_BUFFER, boundsVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(boundsData), boundsData, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLuint boundsEBO;
    glGenBuffers(1, &boundsEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, boundsEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(boundsIndices), boundsIndices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glBindVertexArray(0);
    glDeleteBuffers(1, &boundsVBO);
    glDeleteBuffers(1, &boundsEBO);

    GLuint pointcloundShader;
    createShader(pointcloundShader, pointcloud_vert, pointcloud_frag);

    GLuint shapeShader;
    createShader(shapeShader, shape_vert, shape_frag);

    float ratio;
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    ratio = width / (float) height;

    const glm::vec3 right(1, 0, 0);
    const glm::vec3 up(0, 1, 0);
    const glm::vec3 forward(0, 0, 1);

    glm::vec3 lightDir(0, -1, 0);
    glm::vec4 lightCol(1, 1, 1, 1);
    glm::vec4 ambientCol(0.01, 0.01, 0.01, 1);

    glm::vec4 boundsColor(0, 1, 0, 0.5f);

    glm::vec3 camPos(0, 0, -1);
    glm::quat camRot;
    glm::vec3 move(0);
    glm::vec3 angles(0);

    glm::mat4 projT = glm::perspective(70.f, ratio, 0.1f, 1000.f);
    glm::mat4 viewT = glm::lookAt(camPos, camPos + forward * camRot, up);
    glm::mat4 modelT = glm::scale(glm::vec3(1));
    glm::mat4 mvpT;

    glm::dvec2 mousePos;
    glm::dvec2 mouseDelta;
    glfwGetCursorPos(window, &mousePos.x, &mousePos.y);

    auto start = std::chrono::high_resolution_clock::now();
    auto end = start;

    std::chrono::duration<double, std::nano> frameTime(1.f / 60);
    std::chrono::duration<double, std::nano> elapsed;
    float delta = 0;

    while (!glfwWindowShouldClose(window))
    {
        end = std::chrono::high_resolution_clock::now();
        elapsed = end - start;
        delta = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.f;
        start = end;

        if (elapsed < frameTime)
            std::this_thread::sleep_for(frameTime - elapsed);

        // Input
        mouseDelta = mousePos;
        glfwGetCursorPos(window, &mousePos.x, &mousePos.y);
        mouseDelta = mousePos - mouseDelta;

        move.x = 0;
        move.z = 0;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            move.z = 1;
        else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            move.z = -1;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            move.x = 1;
        else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            move.x = -1;

        // Update
        if (glfwGetMouseButton(window, 0)) {
            angles.x -= mouseDelta.y * delta;
            angles.y += mouseDelta.x * delta;
            camRot = glm::angleAxis(angles.x, right) * glm::angleAxis(angles.y, up);
        }

        if (glm::length(move) > 0)
            glm::normalize(move);
        camPos += 2.f * move * camRot * delta;

        projT = glm::perspective(70.f, ratio, 0.1f, 1000.f);
        viewT = glm::lookAt(camPos, camPos + forward * camRot, up);
        modelT = glm::scale(glm::vec3(2));
        mvpT = projT * viewT * modelT;

        glfwGetFramebufferSize(window, &width, &height);
        ratio = width / (float) height;

        // Draw
        glViewport(0, 0, width, height);

        glClearColor(0.1f, 0.1f, 0.1f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(pointcloundShader);
        glUniformMatrix4fv(glGetUniformLocation(pointcloundShader, "MVP"), 1, GL_TRUE, glm::value_ptr(mvpT));
        glUniform3fv(glGetUniformLocation(pointcloundShader, "LightDir"), 1, glm::value_ptr(lightDir));
        glUniform4fv(glGetUniformLocation(pointcloundShader, "LightCol"), 1, glm::value_ptr(lightCol));
        glUniform4fv(glGetUniformLocation(pointcloundShader, "AmbientCol"), 1, glm::value_ptr(ambientCol));

        const float size = (1.f / pow(glm::length(camPos), 0.8f)) * 20;
        glPointSize(size);
        for (auto mesh : meshes) {
            glBindVertexArray(mesh.first);
            glDrawArrays(GL_POINTS, 0, mesh.second);
        }

        glUseProgram(shapeShader);
        glUniformMatrix4fv(glGetUniformLocation(shapeShader, "MVP"), 1, GL_TRUE, glm::value_ptr(mvpT));
        glUniform4fv(glGetUniformLocation(shapeShader, "Color"), 1, glm::value_ptr(boundsColor));

        glBindVertexArray(bounds);
        glDrawArrays(GL_LINES, 0, sizeof(boundsData) / sizeof(float));
        //glDrawElements(GL_LINES, 3, GL_UNSIGNED_INT, 0);

        /*ImGui_ImplGlfwGL3_NewFrame();
        ImGui::BeginMainMenuBar();

        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("New Scene", "", false, true);
            ImGui::MenuItem("Load Scene", "", false, true);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            ImGui::MenuItem("Undo", "", false, true);
            ImGui::MenuItem("Redo", "", false, true);
            ImGui::MenuItem("Cut", "", false, true);
            ImGui::MenuItem("Copy", "", false, true);
            ImGui::MenuItem("Paste", "", false, true);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
        ImGui::End();
        ImGui::Render();*/

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    for (auto mesh : meshes)
        glDeleteVertexArrays(1, &mesh.first);
    glDeleteVertexArrays(1, &bounds);

    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
