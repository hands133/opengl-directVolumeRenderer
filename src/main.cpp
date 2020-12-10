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
void updateCubeVerts(glm::uvec3& res);
// global data and numerics
// const unsigned int SCR_WIDTH = 1280;
// const unsigned int SCR_HEIGHT = 720;

const unsigned int SCR_WIDTH = 600;
const unsigned int SCR_HEIGHT = 600;

float cubeVerts[48] = {
	-0.5, -0.5, -0.5, 0.0, 0.0, 0.0,
	 0.5, -0.5, -0.5, 1.0, 0.0, 0.0,
	 0.5,  0.5, -0.5, 1.0, 1.0, 0.0,
	-0.5,  0.5, -0.5, 0.0, 1.0, 0.0,
	-0.5, -0.5,  0.5, 0.0, 0.0, 1.0,
	 0.5, -0.5,  0.5, 1.0, 0.0, 1.0,
	 0.5,  0.5,  0.5, 1.0, 1.0, 1.0,
	-0.5,  0.5,  0.5, 0.0, 1.0, 1.0
};

unsigned int indices[] = {
	1, 5, 4,
	4, 0, 1,
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
	0.0, 0.0, 1.0, 1.0 / 6.0,
	0.0, 1.0, 1.0, 2.0 / 6.0,
	0.0, 1.0, 0.0, 0.5,
	1.0, 1.0, 0.0, 4.0 / 6.0,
	1.0, 0.0, 0.0, 5.0 / 6.0,
	0.878431, 0.0, 1.0, 1.0
};

glm::fvec4 bgColor = glm::fvec4(
	82.0 / 255.0,
	87.0 / 255.0,
	110.0 / 255.0,
	255.0 / 255.0);

int main(int argc, char* argv[])
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
	model = glm::rotate(model, glm::radians(45.0f), glm::vec3(1.0, 0.0, 0.0));
	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

	// texture : volume
	rawFile rawfile;
	rawfile.read("..\\..\\datatest\\silicium_98_34_34_uint8.dat");
	// rawfile.read("..\\..\\datatest\\tooth_103x94x161_uint8.dat");
	// rawfile.read("..\\..\\datatest\\data_256x256x256_float.dat");
	
	std::cout << rawfile;

	auto rawRes = rawfile.resolution();
	updateCubeVerts(rawRes);


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


	Texture texVolume("volume", GL_TEXTURE_3D, 0, GL_CLAMP_TO_EDGE, GL_LINEAR, true);

	// only to convert integer-type to float
	if (rawfile.data())
	{
		auto res = rawfile.resolution();
		std::vector<float> buffer;
		rawfile.dataLP(buffer);
		texVolume.setData(GL_R32F, res, 0, GL_RED, GL_FLOAT, &buffer[0]);
	}
	else
	{
		std::cerr << "Failed to load texture" << std::endl;
		return -1;
	}

	Texture texTransFunc("transfer function", GL_TEXTURE_1D, 0, GL_CLAMP_TO_EDGE, GL_LINEAR, true);

	texTransFunc.setData(GL_RGBA, glm::ivec3(7, 0, 0), 0, GL_RGBA, GL_FLOAT, transFunctionf);
	
	Shader shaderIn("../shaders/rayIn_vert.glsl", "../shaders/rayIn_frag.glsl");

	Shader rayCasting("../shaders/rayCasting_vert.glsl", "../shaders/rayCasting_frag.glsl");
	
	rayCasting.use();
	rayCasting.setFloat("SCR_WIDTH", SCR_WIDTH);
	rayCasting.setFloat("SCR_HEIGHT", SCR_HEIGHT);
	rayCasting.setInt("coordIn", projInTexture.getID() - 1);
	rayCasting.setInt("coordOut", projOutTexture.getID() - 1);
	rayCasting.setInt("volume", texVolume.getID() - 1);
	rayCasting.setInt("tFunc", texTransFunc.getID() - 1);

	auto rangeSpan = rawfile.spanValue();
	rayCasting.setFloat("vMin", rangeSpan.x);
	rayCasting.setFloat("vMax", rangeSpan.y);

	glEnable(GL_CULL_FACE);

	while (!glfwWindowShouldClose(window))
	{

		processInput(window);

		glDisable(GL_BLEND);
		glClearColor(bgColor.x, bgColor.y, bgColor.z, bgColor.w);

		model = glm::rotate(model, glm::radians(0.3f), glm::vec3(0.0f, 1.0f, 0.0));

		shaderIn.use();
		shaderIn.setMat4("model", model);
		shaderIn.setMat4("view", view);
		shaderIn.setMat4("projection", projection);

		// render to texture, intro buffer
		projInFBuffer.bind();

		glDepthFunc(GL_LESS);
		glCullFace(GL_BACK);
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

		glClearDepth(1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glBindVertexArray(VAO);

		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, NULL);

		projInFBuffer.unbind();

		// render to texture, outro buffer
		projOutFBuffer.bind();

		glDepthFunc(GL_GREATER);
		glCullFace(GL_FRONT);
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

		glClearDepth(0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shaderIn.use();
		glBindVertexArray(VAO);

		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, NULL);

		projOutFBuffer.unbind();

		// render to screen
		rayCasting.use();
		rayCasting.setMat4("model", model);
		rayCasting.setMat4("view", view);
		rayCasting.setMat4("projection", projection);

		glDepthFunc(GL_LESS);
		glCullFace(GL_BACK);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);

		glClearDepth(1.0);
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

void updateCubeVerts(glm::uvec3& res)
{
	float stdLength = 0.7;

	int maxAxis = std::max({res.x, res.y, res.z});
	float xScale = 1.0 * res.x / maxAxis;
	float yScale = 1.0 * res.y / maxAxis;
	float zScale = 1.0 * res.z / maxAxis;

	float xmin = -stdLength * xScale;
	float xmax = stdLength * xScale;
	float ymin = -stdLength * yScale;
	float ymax = stdLength * yScale;
	float zmin = -stdLength * zScale;
	float zmax = stdLength * zScale;

	float tmpVerts[48] = 
	{
		xmin, ymin, zmin, 0.0, 0.0, 0.0,
		xmax, ymin, zmin, 1.0, 0.0, 0.0,
		xmax, ymax, zmin, 1.0, 1.0, 0.0,
		xmin, ymax, zmin, 0.0, 1.0, 0.0,
		xmin, ymin, zmax, 0.0, 0.0, 1.0,
		xmax, ymin, zmax, 1.0, 0.0, 1.0,
		xmax, ymax, zmax, 1.0, 1.0, 1.0,
		xmin, ymax, zmax, 0.0, 1.0, 1.0
	};

	std::copy(tmpVerts, tmpVerts + 48, cubeVerts);
}