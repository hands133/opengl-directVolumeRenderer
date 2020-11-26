#include <glad/glad.h>
#include <glfw3.h>

#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "rawReader.h"
#include "shader.h"
#include "camera.h"
#include "frameBuffer.h"

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

float transFunctionf[] = {
	0.231371, 0.290839, 0.752941, 0.0,
	0.865003, 0.865003, 0.865003, 0.6,
	0.705882, 0.0156863, 0.14902, 1.0
};

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

	frameBuffer projInFBuffer("project_intro");

	unsigned int framebufferColorTextureIn;
	glGenTextures(1, &framebufferColorTextureIn);
	glBindTexture(GL_TEXTURE_2D, framebufferColorTextureIn);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	projInFBuffer.bindTexture2d(GL_COLOR_ATTACHMENT0, framebufferColorTextureIn, 0);

	unsigned int framebufferDepthTextureIn;
	glGenTextures(1, &framebufferDepthTextureIn);
	glBindTexture(GL_TEXTURE_2D, framebufferDepthTextureIn);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	projInFBuffer.bindTexture2d(GL_DEPTH_ATTACHMENT, framebufferDepthTextureIn, 0);

	projInFBuffer.checkStatus();

	projInFBuffer.bind();
	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

	glClearDepth(1.0);
	glClearColor(0.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	shaderIn.use();
	glBindVertexArray(VAO);

	glDepthFunc(GL_LESS);
	// render to frame buffer
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, NULL);
	// unbind framebuffer to return to default render pipeline
	projInFBuffer.unbind();

	// render out put
	frameBuffer projOutFBuffer("project_outro");

	unsigned int framebufferColorTextureOut;
	glGenTextures(1, &framebufferColorTextureOut);
	glBindTexture(GL_TEXTURE_2D, framebufferColorTextureOut);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	projOutFBuffer.bindTexture2d(GL_COLOR_ATTACHMENT0, framebufferColorTextureOut, 0);

	unsigned int framebufferDepthTextureOut;
	glGenTextures(1, &framebufferDepthTextureOut);
	glBindTexture(GL_TEXTURE_2D, framebufferDepthTextureOut);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	projOutFBuffer.bindTexture2d(GL_DEPTH_ATTACHMENT, framebufferDepthTextureOut, 0);

	projOutFBuffer.checkStatus();
	
	projOutFBuffer.bind();
	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

	glClearDepth(0.0);
	glClearColor(0.0, 0.0, 0.0, 1.0);
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

	unsigned int texVolume;
	glGenTextures(1, &texVolume);
	glBindTexture(GL_TEXTURE_3D, texVolume);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (rawfile.data())
	{
		auto res = rawfile.resolution();
		glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, res.x, res.y, res.z, 0, GL_RED, GL_FLOAT, rawfile.data());
		glGenerateMipmap(GL_TEXTURE_3D);
	}
	else
	{
		std::cerr << "Failed to load texture" << std::endl;
		return -1;
	}

	// texture : transfer function
	unsigned int texTransFunc;
	glGenTextures(1, &texTransFunc);
	glBindTexture(GL_TEXTURE_1D, texTransFunc);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 3, 0, GL_RGBA, GL_FLOAT, transFunctionf);
	//glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 3, 0, GL_RGBA, GL_UNSIGNED_BYTE, transFunctionub);
	glGenerateMipmap(GL_TEXTURE_1D);


	// get color texture here
	// Shader rayCasting("../shaders/rayCasting_vert.glsl", "../shaders/rayCasting_frag.glsl");
	Shader rayCasting("../shaders/rayCasting_vert.glsl", "../shaders/rayCasting_frag_fixPoints.glsl");
	
	rayCasting.use();
	rayCasting.setMat4("model", model);
	rayCasting.setMat4("view", view);
	rayCasting.setMat4("projection", projection);
	rayCasting.setFloat("SCR_WIDTH", SCR_WIDTH);
	rayCasting.setFloat("SCR_HEIGHT", SCR_HEIGHT);
	rayCasting.setInt("coordIn", framebufferColorTextureIn - 1);
	rayCasting.setInt("coordOut", framebufferColorTextureOut - 1);
	rayCasting.setInt("volume", texVolume - 1);
	rayCasting.setInt("tFunc", texTransFunc - 1);

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
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		rayCasting.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, framebufferColorTextureIn);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, framebufferColorTextureOut);

		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_3D, texVolume);

		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_1D, texTransFunc);

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