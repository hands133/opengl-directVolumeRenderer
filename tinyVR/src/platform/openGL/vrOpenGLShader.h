#pragma once

#include "tinyVR/renderer/vrShader.h"
#include <glm/glm.hpp>

namespace tinyvr {

	class vrOpenGLShader : public vrShader
	{
	public:
		vrOpenGLShader(const std::string& vertexSrc, const std::string& fragmentSrc);
		virtual ~vrOpenGLShader();

		virtual void Bind() const override;
		virtual void Unbind() const override;

		void SetBool(const std::string& name, bool value) override;
		void SetInt(const std::string& name, int value) override;
		void SetInt2(const std::string& name, const glm::ivec2& values) override;
		void SetInt3(const std::string& name, const glm::ivec3& values) override;
		void SetInt4(const std::string& name, const glm::ivec4& values) override;
		void SetInts(const std::string& name, int* valuePtr, size_t items) override;

		void SetFloat(const std::string& name, float value) override;
		void SetFloat2(const std::string& name, const glm::vec2& values) override;
		void SetFloat3(const std::string& name, const glm::vec3& values) override;
		void SetFloat4(const std::string& name, const glm::vec4& values) override;
		void SetFloats(const std::string& name, float* valuePtr, size_t items) override;

		void SetMat3(const std::string& name, const glm::mat3& matrix) override;
		void SetMat4(const std::string& name, const glm::mat4& matrix) override;

		void SetTexture(const std::string& name, const vrRef<vrTexture>& tex) override;

	private:
		uint32_t m_RendererID;
		int m_MaxTextureUnit;
	};
}

