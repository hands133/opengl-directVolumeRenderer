#include "vrpch.h"
#include "vrRenderer.h"

#include "platform/openGL/vrOpenGLShader.h"

namespace tinyvr {

	vrRenderer::SceneData* vrRenderer::m_SceneData = new vrRenderer::SceneData();

	void vrRenderer::Init()
	{
		vrRenderCommand::Init();
	}

	void vrRenderer::Shutdown()
	{
	}

	void vrRenderer::OnWindowResize(uint32_t width, uint32_t height)
	{
		vrRenderCommand::SetViewPort(0, 0, width, height);
	}

	void vrRenderer::BeginScene(vrCamera& camera)
	{
		m_SceneData->ViewProjectionMatrix = camera.GetViewProjectionMatrix();
	}

	void vrRenderer::EndScene()
	{
	}

	void vrRenderer::Submit(const vrRef<vrShader>& shader, const vrRef<vrVertexArray>& vertexArray, const glm::mat4& transform)
	{
		shader->Bind();
		std::dynamic_pointer_cast<vrOpenGLShader>(shader)->SetMat4("u_ViewProjection", m_SceneData->ViewProjectionMatrix);
		std::dynamic_pointer_cast<vrOpenGLShader>(shader)->SetMat4("u_Transform", transform);

		vertexArray->Bind();
		vrRenderCommand::DrawIndexed(vertexArray);
	}

}