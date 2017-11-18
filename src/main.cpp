#include <glad/glad.h>

#include <GLFW/glfw3.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <chrono>
#include <thread>

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

static const char *vertSrc =
	"#version 330 core\n\
	layout(location = 0) in vec3 POSITION;\n\
	layout(location = 1) in vec3 NORMAL;\n\
	out vec3 _Normal;\n\
	uniform mat4 MVP;\n\
	\n\
	void main() {\n\
		gl_Position = vec4(POSITION, 1.0) * MVP;\n\
		_Normal = NORMAL;\n\
	}\n";

static const char *fragSrc =
	"#version 330 core\n\
	in vec3 _Normal;\n\
	out vec4 frag;\n\
	uniform vec3 LightDir;\n\
	uniform vec4 LightCol;\n\
	uniform vec4 AmbientCol;\n\
	\n\
	void main() {\n\
		float d = dot(_Normal, LightDir);\n\
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

	for (size_t i = 0; i < shapes.size(); i++) {
		size_t posCount = shapes[i].mesh.positions.size();
		size_t norCount = shapes[i].mesh.normals.size();
		GLuint mesh = 0;
		GLuint posVBO = 0;
		GLuint norVBO = 0;

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

	GLint success = 0;
	GLchar errBuff[1024] = { 0 };

	GLuint vert = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vert, 1, &vertSrc, NULL);
	glCompileShader(vert);

	glGetShaderiv(vert, GL_COMPILE_STATUS, &success);
	if (success == GL_FALSE) {
		glGetShaderInfoLog(vert, sizeof(errBuff), NULL, errBuff);
		std::cerr << errBuff << std::endl;
	}

	GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(frag, 1, &fragSrc, NULL);
	glCompileShader(frag);

	glGetShaderiv(frag, GL_COMPILE_STATUS, &success);
	if (success == GL_FALSE) {
		glGetShaderInfoLog(frag, sizeof(errBuff), NULL, errBuff);
		std::cerr << errBuff << std::endl;
	}

	unsigned int shader = glCreateProgram();
	glAttachShader(shader, vert);
	glAttachShader(shader, frag);

	glLinkProgram(shader);

	glGetProgramiv(shader, GL_LINK_STATUS, &success);
	if (success == GL_FALSE) {
		glGetShaderInfoLog(frag, sizeof(errBuff), NULL, errBuff);
		std::cerr << errBuff << std::endl;
	}

	glValidateProgram(shader);

	glGetProgramiv(shader, GL_LINK_STATUS, &success);
	if (success == GL_FALSE) {
		glGetShaderInfoLog(frag, sizeof(errBuff), NULL, errBuff);
		std::cerr << errBuff << std::endl;
	}

	glDeleteShader(vert);
	glDeleteShader(frag);

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
		angles.x -= mouseDelta.y * delta;
		angles.y += mouseDelta.x * delta;
		camRot = glm::angleAxis(angles.x, right) * glm::angleAxis(angles.y, up);

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

		glUseProgram(shader);
		glUniformMatrix4fv(glGetUniformLocation(shader, "MVP"), 1, GL_TRUE, glm::value_ptr(mvpT));
		glUniform3fv(glGetUniformLocation(shader, "LightDir"), 1, glm::value_ptr(lightDir));
		glUniform4fv(glGetUniformLocation(shader, "LightCol"), 1, glm::value_ptr(lightCol));
		glUniform4fv(glGetUniformLocation(shader, "AmbientCol"), 1, glm::value_ptr(ambientCol));

		const float size = (1.f / pow(glm::length(camPos), 0.8f)) * 20;
		glPointSize(size);
		for (auto mesh : meshes) {
			glBindVertexArray(mesh.first);
			glBindBuffer(GL_ARRAY_BUFFER, mesh.first);
			glDrawArrays(GL_POINTS, 0, mesh.second);
		}

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

	for (auto mesh : meshes)
		glDeleteVertexArrays(1, &mesh.first);

    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
