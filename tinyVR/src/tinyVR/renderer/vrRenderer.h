#pragma once

#include "tinyVR/core/vrBase.h"

#include "vrRenderCommand.h"

#include "tinyVR/renderer/vrCamera.h"
#include "vrShader.h"

namespace tinyvr {

	class vrRenderer
	{
	public:
		static void Init();
		static void Shutdown();

		static void OnWindowResize(uint32_t width, uint32_t height);

		static void BeginScene(vrCamera& camera);
		static void EndScene();

		static void Submit(const vrRef<vrShader>& shader, const vrRef<vrVertexArray>& vertexArray, const glm::mat4& transform = glm::mat4(1.0f));

		static vrRendererAPI::API GetAPI() { return vrRendererAPI::GetAPI(); }
	private:
		struct SceneData
		{
			glm::mat4 ViewProjectionMatrix;
		};

		static SceneData* m_SceneData;
	};
}

