#include <glad/glad.h>
#include <glfw3.h>

#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "rawReader.h"
#include "shader.h"
#include "camera.h"
#include "frameBuffer.h"
#include "texture.h"

// call back functions

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

// global data and numerics
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

float xmin = -0.5;
float xmax = 0.5;
float ymin = -0.5;
float ymax = 0.5;
float zmin = -0.5;
float zmax = 0.5;

float cubeVerts[] = {
	xmin, ymin, zmin, 0.0, 0.0, 0.0,
	xmax, ymin, zmin, 1.0, 0.0, 0.0,
	xmax, ymax, zmin, 1.0, 1.0, 0.0,
	xmin, ymax, zmin, 0.0, 1.0, 0.0,
	xmin, ymin, zmax, 0.0, 0.0, 1.0,
	xmax, ymin, zmax, 1.0, 0.0, 1.0,
	xmax, ymax, zmax, 1.0, 1.0, 1.0,
	xmin, ymax, zmax, 0.0, 1.0, 1.0
};

unsigned int indices[] = {
	1, 0, 4,
	4, 5, 1,
	3, 7, 6,
	6, 2, 3,
	3, 0, 4,
	4, 7, 3,
	1, 2, 6,
	6, 5, 1,
	4, 5, 6,
	6, 7, 4,
	1, 0, 3,
	3, 2, 1
};

unsigned char transFunctionub[] = {
	59, 74, 192, 0,
	221, 221, 221, 128,
	180, 4, 38, 255
};

// float transFunctionf[] = {
// 	0.231371, 0.290839, 0.752941, 0.0,
// 	0.865003, 0.865003, 0.865003, 0.6,
// 	0.705882, 0.0156863, 0.14902, 1.0
// };
float transFunctionf[] = {
	1.0, 1.0, 1.0, 0.0,
	0.0, 0.0, 1.0, 0.16666666,
	0.0, 1.0, 1.0, 0.33333333,
	0.0, 1.0, 0.0, 0.5,
	1.0, 1.0, 0.0, 0.66666666,
	1.0, 0.0, 0.0, 0.83333333,
	0.878431, 0.0, 1.0
};

glm::fvec4 bgColor = glm::fvec4(
	82.0 / 255.0,
	87.0 / 255.0,
	110.0 / 255.0,
	255.0 / 255.0);

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Volume Ray Casting", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// camera
	Camera camera(glm::vec3(0.0, 0.0, 3.0));

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0, 1.0, 0.0));
	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

	// VAO, VBO, EBO
	// send vertex point data to graphic pipeline : vertex shader
	unsigned int VAO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	unsigned int VBO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	unsigned int EBO;
	glGenBuffers(1, &EBO);

	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVerts), cubeVerts, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// tell openGL to analyse the vertex data
	// (location, vertex size(vec3), type, if normalize, stride(step), offset(0))
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), NULL);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// render to texture
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);

	Shader shaderIn("../shaders/rayIn_vert.glsl", "../shaders/rayIn_frag.glsl");

	shaderIn.use();
	shaderIn.setMat4("model", model);
	shaderIn.setMat4("view", view);
	shaderIn.setMat4("projection", projection);

	// frame buffer for intro point
	frameBuffer projInFBuffer("project_intro");
	// color texture (store intro point)
	Texture projInTexture("project_intro::color", GL_TEXTURE_2D, 0, GL_CLAMP_TO_EDGE, GL_LINEAR);
	projInTexture.setData(GL_RGBA, glm::ivec3(SCR_WIDTH, SCR_HEIGHT, 0), 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	// depth texture (store depth)
	Texture projInDepthTexture("proj_intro::depth", GL_TEXTURE_2D, 0, GL_CLAMP_TO_EDGE, GL_LINEAR);
	projInDepthTexture.setData(GL_DEPTH_COMPONENT32F, glm::ivec3(SCR_WIDTH, SCR_HEIGHT, 0), 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	projInFBuffer.bindTexture2d(GL_COLOR_ATTACHMENT0, projInTexture.getID(), projInTexture.getLvl());
	projInFBuffer.bindTexture2d(GL_DEPTH_ATTACHMENT, projInDepthTexture.getID(), projInDepthTexture.getLvl());

	projInFBuffer.checkStatus();

	projInFBuffer.bind();

	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

	glClearDepth(1.0);
	glClearColor(0.0, 1.0, 1.0, 1.0);
	glClearColor(bgColor.x, bgColor.y, bgColor.z, bgColor.w);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	shaderIn.use();
	glBindVertexArray(VAO);

	glDepthFunc(GL_LESS);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, NULL);
	// unbind framebuffer to return to default render pipeline
	projInFBuffer.unbind();

	// render out put
	frameBuffer projOutFBuffer("project_outro");

	// color texture (store intro point)
	Texture projOutTexture("project_outro::color", GL_TEXTURE_2D, 0, GL_CLAMP_TO_EDGE, GL_LINEAR);
	projOutTexture.setData(GL_RGBA, glm::ivec3(SCR_WIDTH, SCR_HEIGHT, 0), 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	// depth texture (store depth)
	Texture projOutDepthTexture("proj_outro::depth", GL_TEXTURE_2D, 0, GL_CLAMP_TO_EDGE, GL_LINEAR);
	projOutDepthTexture.setData(GL_DEPTH_COMPONENT32F, glm::ivec3(SCR_WIDTH, SCR_HEIGHT, 0), 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	projOutFBuffer.bindTexture2d(GL_COLOR_ATTACHMENT0, projOutTexture.getID(), projOutTexture.getLvl());
	projOutFBuffer.bindTexture2d(GL_DEPTH_ATTACHMENT, projOutDepthTexture.getID(), projOutDepthTexture.getLvl());

	projOutFBuffer.checkStatus();

	projOutFBuffer.bind();

	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

	glClearDepth(0.0);
	glClearColor(bgColor.x, bgColor.y, bgColor.z, bgColor.w);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	shaderIn.use();
	glBindVertexArray(VAO);

	glDepthFunc(GL_GREATER);
	// render to frame buffer
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, NULL);

	// unbind framebuffer to return to default render pipeline
	projOutFBuffer.unbind();

	 //texture : volume
	rawFile rawfile;
	rawfile.read("C:\\Users\\hands33\\Desktop\\Bachelor\\Computer Graphics\\project\\src\\proj1\\data_256x256x256_float.dat");
	std::cout << rawfile;

	Texture texVolume("volume", GL_TEXTURE_3D, 0, GL_CLAMP_TO_EDGE, GL_LINEAR, true);

	if (rawfile.data())
	{
		auto res = rawfile.resolution();
		texVolume.setData(GL_R32F, res, 0, GL_RED, GL_FLOAT, rawfile.data());
	}
	else
	{
		std::cerr << "Failed to load texture" << std::endl;
		return -1;
	}

	Texture texTransFunc("transfer function", GL_TEXTURE_1D, 0, GL_CLAMP_TO_EDGE, GL_LINEAR, true);

	texTransFunc.setData(GL_RGBA, glm::ivec3(7, 0, 0), 0, GL_RGBA, GL_FLOAT, transFunctionf);
	//glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 3, 0, GL_RGBA, GL_UNSIGNED_BYTE, transFunctionub);

	Shader rayCasting("../shaders/rayCasting_vert.glsl", "../shaders/rayCasting_frag_fixPoints.glsl");
	
	rayCasting.use();
	rayCasting.setMat4("model", model);
	rayCasting.setMat4("view", view);
	rayCasting.setMat4("projection", projection);
	rayCasting.setFloat("SCR_WIDTH", SCR_WIDTH);
	rayCasting.setFloat("SCR_HEIGHT", SCR_HEIGHT);
	rayCasting.setInt("coordIn", projInTexture.getID() - 1);
	rayCasting.setInt("coordOut", projOutTexture.getID() - 1);
	rayCasting.setInt("volume", texVolume.getID() - 1);
	rayCasting.setInt("tFunc", texTransFunc.getID() - 1);

	auto rangeSpan = rawfile.spanValue();
	rayCasting.setFloat("vMin", rangeSpan.x);
	rayCasting.setFloat("vMax", rangeSpan.y);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);

	glEnable(GL_CULL_FACE);
	// glCullFace(GL_BACK);

	while (!glfwWindowShouldClose(window))
	{
		processInput(window);

		glDepthFunc(GL_GREATER);
		glClearDepth(0.0);
		glClearColor(bgColor.x, bgColor.y, bgColor.z, bgColor.w);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		rayCasting.use();
		glActiveTexture(GL_TEXTURE0);
		projInTexture.bind();

		glActiveTexture(GL_TEXTURE2);
		projOutTexture.bind();

		glActiveTexture(GL_TEXTURE4);
		texVolume.bind();

		glActiveTexture(GL_TEXTURE5);
		texTransFunc.bind();

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, NULL);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteBuffers(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);

	glfwTerminate();                // collect resource

	return 0;
}

void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}