#include "vrpch.h"
#include "vrShader.h"

#include "vrRenderer.h"
#include "platform/openGL/vrOpenGLShader.h"
#include "platform/openGL/vrOpenGLCompShader.h"

#include <fstream>

namespace tinyvr {

	vrRef<vrShader> vrShader::Create(const std::string& vertexSrcPath, const std::string& fragmentSrcPath)
	{
		std::string vertCodeSrc;
		std::string fragCodeSrc;

		std::ifstream vShaderFile;
		std::ifstream fShaderFile;

		TINYVR_ASSERT(std::filesystem::exists(vertexSrcPath), fmt::format("Vertex Shader doesn't exist : {0}", vertexSrcPath));
		TINYVR_ASSERT(std::filesystem::exists(fragmentSrcPath), fmt::format("Frag Shader doesn't exist : {0}", fragmentSrcPath));

		// throw exception
		vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		try
		{
			// open file
			vShaderFile.open(vertexSrcPath);
			fShaderFile.open(fragmentSrcPath);
			std::stringstream vShaderStream, fShaderStream;
			// read from file to buffer
			vShaderStream << vShaderFile.rdbuf();
			fShaderStream << fShaderFile.rdbuf();
			// close file
			vShaderFile.close();
			fShaderFile.close();
			// convert to string
			vertCodeSrc = vShaderStream.str();
			fragCodeSrc = fShaderStream.str();
		}
		catch (const std::ifstream::failure& e)
		{
			std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ\n" << e.what() << '\n';
			vShaderFile.close();
			fShaderFile.close();
			return nullptr;
		}

		switch (vrRenderer::GetAPI())
		{
		case vrRendererAPI::API::None:
			TINYVR_ASSERT(false, "RendererAPI::None is currently not supported!");
			return nullptr;
		case vrRendererAPI::API::OpenGL:	return std::make_shared<vrOpenGLShader>(vertCodeSrc, fragCodeSrc);
		}

		TINYVR_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	vrRef<vrShader> vrShader::CreateComp(const std::string& compSrcPath)
	{
		std::string compCodeSrc;
		std::ifstream cShaderFile;

		TINYVR_ASSERT(std::filesystem::exists(compSrcPath), fmt::format("Compute Shader Doesn't exist : {0}", compSrcPath));

		// throw exception
		cShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		try
		{
			// open file
			cShaderFile.open(compSrcPath);
			std::stringstream cShaderStream;
			// read from file to buffer
			cShaderStream << cShaderFile.rdbuf();
			// close file
			cShaderFile.close();
			// convert to string
			compCodeSrc = cShaderStream.str();
		}
		catch (const std::ifstream::failure& e)
		{
			std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ\n" << e.what() << '\n';
			cShaderFile.close();
			return nullptr;
		}

		switch (vrRenderer::GetAPI())
		{
		case vrRendererAPI::API::None:
			TINYVR_ASSERT(false, "RendererAPI::None is currently not supported!");
			return nullptr;
		case vrRendererAPI::API::OpenGL:	return std::make_shared<vrOpenGLCompShader>(compCodeSrc);
		}

		TINYVR_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}
}
