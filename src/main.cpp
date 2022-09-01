#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <chrono>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "rawReader.h"
#include "shader.h"
#include "camera.h"
#include "frameBuffer.h"
#include "texture.h"
#include "util.h"
#include "ds_pod.h"
#include "trackBall_CHEN.h"

// call back functions

void processInput(GLFWwindow *window);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void click_callback(GLFWwindow *window, int button, int action, int mods);

// void updateCubeVerts(glm::uvec3& res, Vertex* pVerts);
// global data and numerics
// const unsigned int SCR_WIDTH = 1920;
// const unsigned int SCR_HEIGHT = 1080;

// const unsigned int SCR_WIDTH = 1280;
// const unsigned int SCR_HEIGHT = 720;

// const unsigned int SCR_WIDTH = 1000;
// const unsigned int SCR_HEIGHT = 1000;

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 800;

// const unsigned int SCR_WIDTH = 600;
// const unsigned int SCR_HEIGHT = 600;

Vertex cubeVerts[8] = 
{
	{ { -0.5, -0.5, -0.5 }, { 0.0, 0.0, 0.0, } },
	{ {  0.5, -0.5, -0.5 }, { 1.0, 0.0, 0.0, } },
	{ {  0.5,  0.5, -0.5 }, { 1.0, 1.0, 0.0, } },
	{ { -0.5,  0.5, -0.5 }, { 0.0, 1.0, 0.0, } },
	{ { -0.5, -0.5,  0.5 }, { 0.0, 0.0, 1.0, } },
	{ {  0.5, -0.5,  0.5 }, { 1.0, 0.0, 1.0, } },
	{ {  0.5,  0.5,  0.5 }, { 1.0, 1.0, 1.0, } },
	{ { -0.5,  0.5,  0.5 }, { 0.0, 1.0, 1.0  } }
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

float transFunctionf[] = {
	1.0, 1.0, 1.0, 0.0 / 6.0,
	0.0, 0.0, 1.0, 1.0 / 6.0,
	0.0, 1.0, 1.0, 2.0 / 6.0,
	0.0, 1.0, 0.0, 3.0 / 6.0,
	1.0, 1.0, 0.0, 4.0 / 6.0,
	1.0, 0.0, 0.0, 5.0 / 6.0,
	0.878431, 0.0, 1.0, 1.0
};

glm::fvec4 bgColor = glm::fvec4(
	82.0 / 255.0,
	87.0 / 255.0,
	110.0 / 255.0,
	255.0 / 255.0);

// camera
Camera camera(glm::vec3(0.0, 0.0, 3.0));
std::unique_ptr<TrackBall_CHEN> ptrackBall = nullptr;

bool mouseClicked = false;

// maybe matrix model could be part of the class renderer
// in the future
glm::mat4 model = glm::mat4(1.0);

int main(int argc, char* argv[])
{
	std::cout << "Excute dir : " << argv[0] << std::endl;
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
	// register function here
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetMouseButtonCallback(window, click_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if(0 == gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

	// texture : volume
    std::string datPath = "";
	rawFile rawfile;
	// datPath = "..\\..\\datatest\\silicium_98_34_34_uint8.dat";
	// datPath = "..\\..\\datatest\\tooth_103x94x161_uint8.dat";
	// datPath = "..\\..\\datatest\\fuel_64x64x64_uint8.dat";
	// datPath = "..\\..\\datatest\\data_256x256x256_float.dat";
	datPath = "..\\..\\datatest\\aneurism_256x256x256_uint8.dat";
	// datPath = "..\\..\\datatest\\bonsai_256x256x256_uint8.dat";
    bool readSuccess = rawfile.read(datPath);

    if (!readSuccess)
    {
        std::cout << "Read file " << datPath << " Failed" << std::endl;
        return -1;
    }

    // ptrackBall = std::make_unique<TrackBall_CHEN>(new TrackBall_CHEN());
    ptrackBall = std::unique_ptr<TrackBall_CHEN>(new TrackBall_CHEN());
	
	std::cout << rawfile;

    model = rawfile.getModelMat();
	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.01f, 100.0f);

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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
        sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, Vertex::P)));
	glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
        sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, Vertex::T)));
	glEnableVertexAttribArray(1);

	// render to texture
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);

	// frame buffer for intro point
	frameBuffer projInFBuffer("project_intro");
	// color texture (store intro point)
	Texture projInTexture("project_intro::color", GL_TEXTURE_2D, 0, GL_CLAMP_TO_EDGE, GL_LINEAR);
	projInTexture.setData(GL_RGBA32F, glm::ivec3(SCR_WIDTH, SCR_HEIGHT, 0), 0, GL_RGBA, GL_FLOAT, NULL);
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
	projOutTexture.setData(GL_RGBA32F, glm::ivec3(SCR_WIDTH, SCR_HEIGHT, 0), 0, GL_RGBA, GL_FLOAT, NULL);
	// depth texture (store depth)
	Texture projOutDepthTexture("proj_outro::depth", GL_TEXTURE_2D, 0, GL_CLAMP_TO_EDGE, GL_LINEAR);
	projOutDepthTexture.setData(GL_DEPTH_COMPONENT32F, glm::ivec3(SCR_WIDTH, SCR_HEIGHT, 0), 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	projOutFBuffer.bindTexture2d(GL_COLOR_ATTACHMENT0, projOutTexture.getID(), projOutTexture.getLvl());
	projOutFBuffer.bindTexture2d(GL_DEPTH_ATTACHMENT, projOutDepthTexture.getID(), projOutDepthTexture.getLvl());

	projOutFBuffer.checkStatus();

    std::vector<float> buffer;
    rawfile.dataLP(buffer);

	Texture texVolume("volume", GL_TEXTURE_3D, 0, GL_CLAMP_TO_EDGE, GL_LINEAR, true);
    texVolume.setData(GL_RG32F, rawfile.resolution(), 0, GL_RED, GL_FLOAT, buffer.data());

	Texture texTransFunc("transfer function", GL_TEXTURE_1D, 0, GL_CLAMP_TO_EDGE, GL_LINEAR, true);
	texTransFunc.setData(GL_RGBA, glm::ivec3(7, 0, 0), 0, GL_RGBA, GL_FLOAT, transFunctionf);
	
	Shader projShader("../resources/shaders/rayIn_vert.glsl", "../resources/shaders/rayIn_frag.glsl");
	Shader rcShader("../resources/shaders/rayCasting_vert.glsl", "../resources/shaders/rayCasting_frag.glsl");
	
	glEnable(GL_CULL_FACE);
	
	uint64_t count = 1;
	while (!glfwWindowShouldClose(window))
	{
		auto start = std::chrono::steady_clock::now();
		processInput(window);

		view = camera.GetViewMatrix();
		projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.01f, 100.0f);

		glDisable(GL_BLEND);
		glClearColor(bgColor.x, bgColor.y, bgColor.z, bgColor.w);

		projShader.use();
		projShader.setMat4("model", ptrackBall->getDragMat() * model);
		projShader.setMat4("view", view);
		projShader.setMat4("projection", projection);

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

		projShader.use();
		glBindVertexArray(VAO);

		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, NULL);

		projOutFBuffer.unbind();

		// render to screen
        glDepthFunc(GL_LESS);
		glCullFace(GL_BACK);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);

		glClearDepth(1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		rcShader.use();
		rcShader.setMat4("model", ptrackBall->getDragMat() * model);
		rcShader.setMat4("view", view);
		rcShader.setMat4("projection", projection);

        rcShader.setFloat("SCR_WIDTH", SCR_WIDTH);
        rcShader.setFloat("SCR_HEIGHT", SCR_HEIGHT);

    	auto rangeSpan = rawfile.spanValue();

        rcShader.setFloat("vMin", rangeSpan.x);
        rcShader.setFloat("vMax", rangeSpan.y);

        rcShader.setTexture("coordIn", projInTexture);
        rcShader.setTexture("coordOut", projOutTexture);
        rcShader.setTexture("volume", texVolume);
        rcShader.setTexture("tFunc", texTransFunc);

		glBindVertexArray(VAO);

		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, NULL);

		glfwSwapBuffers(window);
		glfwPollEvents();
		auto end = std::chrono::steady_clock::now();
		if(count % 5 == 0)
		{
			auto tt = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

			float fps = 1000 / (1.0 * tt.count());

			std::cout << "Delay = " << tt.count() << " ms, FPS = " << fps << "\r";
		}
		++count;
	}

    std::cout << std::endl;

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

// NOTE: every parameter (pos) passed by const reference,
// should be converted from screen coordinates to image coordinates,
// which the origin point on the left-top corner on the screen, now
// should be put on the center of the screen.

// The function screen2Image shouldn't be member function,
// especially not the part of the controller.

inline glm::dvec2 screen2Image(glm::dvec2 pos, glm::uvec2 res)
{
    glm::dvec2 imageCoord(
        2.0 * pos.x / res.x - 1.0,
        -2.0 * pos.y / res.y + 1.0);
    return imageCoord;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    auto cursor2Image = screen2Image(
        glm::dvec2(xpos, ypos),
        glm::uvec2(SCR_WIDTH, SCR_HEIGHT));

    ptrackBall->setCurrentPos(cursor2Image);
    // controller.setCurrent(cursor2Image);
}

void click_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		if (button == GLFW_MOUSE_BUTTON_1)
		{
            ptrackBall->setPressPos();
            // controller.setPressed();
		}
	}
	else if (action == GLFW_RELEASE)
	{
		if (button == GLFW_MOUSE_BUTTON_1)
		{
            // model = controller.getDragMat() * model;
            model = ptrackBall->getDragMat() * model;
            // controller.setRelease();
            ptrackBall->setReleasePos();
		}
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}