#include "vrIOTLayer.h"

#include "imgui/imgui.h"

#include "tinyVR/renderer/vrShader.h"

namespace iot {

	IOTLayer::IOTLayer(const std::string& path) :
		vrLayer("Independent Opacity Transparent Layer"),
		m_ScrSize(1280, 720), m_Path(path), m_ModelMat(glm::mat4(1.0f)),
		zeroFillerVec(0.0f), oneFillerVec(1.0f), m_Opacity(1.0f)
	{
		m_CameraController.reset(new tinyvr::vrPerspectiveCameraController(
			{ 0.0f, 0.0f, 2.0f }, m_ScrSize.x, m_ScrSize.y));
		{
			m_BGColor = glm::vec4(82.0f, 87.0f, 110.0f, 255.0f) / glm::vec4(255.0f);
			m_TrackBall.reset(new tinyvr::vrTrackBall_CHEN(m_MousePos));

			tinyvr::vrRenderCommand::ShadeModel();
			tinyvr::vrRenderCommand::EnableMultiSample();
		}

		{
			float quadVertices[] = {
				-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
				 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
				 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
				-1.0f,  1.0f, 0.0f, 0.0f, 1.0f
			};

			quadVB = tinyvr::vrVertexBuffer::Create(quadVertices, sizeof(quadVertices));
			quadVB->SetLayout({
				{ tinyvr::ShaderDataType::Float3, "position" },
				{ tinyvr::ShaderDataType::Float2, "uv" }
				});

			uint32_t quadIndices[] = {
				0, 1, 2,
				2, 3, 0
			};

			quadIB = tinyvr::vrIndexBuffer::Create(quadIndices, sizeof(quadIndices) / sizeof(uint32_t));

			quadVA = tinyvr::vrVertexArray::Create();
			quadVA->Bind();
			quadVA->AddVertexBuffer(quadVB);
			quadVA->SetIndexBuffer(quadIB);
		}

		{
			std::string resBasePath = "resources/";
			std::string shaderPath = resBasePath + "shaders/";
			std::string modelPath = resBasePath + "models/";

			lightingShader = tinyvr::vrShader::Create(shaderPath + "materials.vs", shaderPath + "materials.fs");
			compositeShader = tinyvr::vrShader::Create(shaderPath + "composite.vs", shaderPath + "composite.fs");
			screenShader = tinyvr::vrShader::Create(shaderPath + "screen.vs", shaderPath + "screen.fs");

			//std::string path = modelPath + "obj/nanosuit.obj";
			//std::string path = modelPath + "obj/su_27.obj";
			//std::string path = modelPath + "obj/tank.obj";
			std::string path = modelPath + "obj/trent1000.obj";

			//std::string path = modelPath + "ply/hand.ply";
			//std::string path = modelPath + "ply/dragon_vrip.ply";
			//std::string path = modelPath + "ply/Armadillo.ply";
			//std::string path = modelPath + "ply/fandisk.ply";
			//std::string path = modelPath + "ply/monkey.ply";
			//std::string path = modelPath + "ply/xyzrgb_statuette.ply";
			//std::string path = modelPath + "ply/lucy.ply";

			myModel.LoadModel(path);

			{
				auto bbx = myModel.BBX;

				TINYVR_INFO("Bounding box of model {0}\nSpatial Span:[{1:>8.5},{2:>8.5},{3:>8.5}] ~ [{4:>8.5},{5:>8.5},{6:>8.5}]\n",
					path, bbx.mMin.x, bbx.mMin.y, bbx.mMin.z, bbx.mMax.x, bbx.mMax.y, bbx.mMax.z);

				auto center = (bbx.mMax + bbx.mMin) / 2.0f;
				glm::vec3 spatialCenter(center.x, center.y, center.z);
				auto transMat = glm::translate(glm::mat4(1.0f), -spatialCenter);

				auto span = bbx.mMax - bbx.mMin;
				glm::vec3 spatialSpan(span.x, span.y, span.z);
				float maxAxis = std::max({ spatialSpan.x, spatialSpan.y, spatialSpan.z });
				auto Scale = 1.0f / glm::vec3(maxAxis);

				auto scaleMat = glm::scale(glm::mat4(1.0f), Scale);
				m_ModelMat = scaleMat * transMat;
			}
		}

		{
			opaqueTexture = tinyvr::vrTexture2D::Create(m_ScrSize.x, m_ScrSize.y,
				tinyvr::vrTextureFormat::TEXTURE_FMT_RGBA, tinyvr::vrTextureType::TEXTURE_TYPE_FLT16);
			opaqueTexture->SetData(m_ScrSize.x * m_ScrSize.y, nullptr);

			depthTexture = tinyvr::vrTexture2D::Create(m_ScrSize.x, m_ScrSize.y,
				tinyvr::vrTextureFormat::TEXTURE_FMT_DEPTH, tinyvr::vrTextureType::TEXTURE_TYPE_FLT32);
			depthTexture->SetData(m_ScrSize.x * m_ScrSize.y, nullptr);

			accumTexture = tinyvr::vrTexture2D::Create(m_ScrSize.x, m_ScrSize.y,
				tinyvr::vrTextureFormat::TEXTURE_FMT_RGBA, tinyvr::vrTextureType::TEXTURE_TYPE_FLT16);
			accumTexture->SetData(m_ScrSize.x * m_ScrSize.y, nullptr);

			revealTexture = tinyvr::vrTexture2D::Create(m_ScrSize.x, m_ScrSize.y,
				tinyvr::vrTextureFormat::TEXTURE_FMT_RED, tinyvr::vrTextureType::TEXTURE_TYPE_FLT32);
			revealTexture->SetData(m_ScrSize.x * m_ScrSize.y, nullptr);
		}

		{
			tinyvr::vrFrameBufferSpecification spec;
			spec.Width = m_ScrSize.x;
			spec.Height = m_ScrSize.y;

			opaqueFBO = tinyvr::vrFrameBuffer::Create(spec);
			opaqueFBO->Bind();
			opaqueFBO->BindTexture(tinyvr::vrFBAttach::COLOR_ATTACHMENT, opaqueTexture, 0);
			opaqueFBO->BindTexture(tinyvr::vrFBAttach::DEPTH_ATTACHMENT, depthTexture);
			opaqueFBO->CheckStatus();
			opaqueFBO->Unbind();


			transparentFBO = tinyvr::vrFrameBuffer::Create(spec);
			transparentFBO->Bind();
			tinyvr::vrRenderCommand::MapFBBuffer(2);
			transparentFBO->BindTexture(tinyvr::vrFBAttach::COLOR_ATTACHMENT, accumTexture, 0);
			transparentFBO->BindTexture(tinyvr::vrFBAttach::COLOR_ATTACHMENT, revealTexture, 1);
			transparentFBO->BindTexture(tinyvr::vrFBAttach::DEPTH_ATTACHMENT, depthTexture);
			transparentFBO->CheckStatus();
			transparentFBO->Unbind();
		}
	}

	void IOTLayer::OnUpdate(tinyvr::vrTimestep ts)
	{
		if (!m_TrackBall->IsMousePressed() && tinyvr::vrInput::IsMouseButtonPressed(TINYVR_MOUSE_BUTTON_LEFT))
			m_TrackBall->mousePressed();

		if (m_TrackBall->IsMousePressed())
		{
			if (tinyvr::vrInput::IsMouseButtonReleased(TINYVR_MOUSE_BUTTON_LEFT))
			{
				m_ModelMat = m_TrackBall->getDragMat() * m_ModelMat;
				m_TrackBall->mouseReleased();
			}
			else
				m_TrackBall->mouseMove();
		}

		auto scr2Image = [&](glm::vec2 p) -> glm::vec2
		{
			return { 2.0 * p.x / m_ScrSize.x - 1.0,
					-2.0 * p.y / m_ScrSize.y + 1.0 };
		};

		auto [mx, my] = tinyvr::vrInput::GetMousePosition();
		m_MousePos = scr2Image({ mx, my });

		tinyvr::vrRenderer::BeginScene(m_CameraController->GetCamera());

		{
			tinyvr::vrRenderCommand::EnableDepthTest(tinyvr::vrAPIOPDepth::DEPTH_LESS);
			tinyvr::vrRenderCommand::DisableAlphaBlend();
			//tinyvr::vrRenderCommand::SetClearColor(glm::vec4(0.0f));
			tinyvr::vrRenderCommand::SetClearColor(m_BGColor);

			opaqueFBO->Bind();
			tinyvr::vrRenderCommand::Clear();
			opaqueFBO->Unbind();
		}

		{
			transparentFBO->Bind();

			tinyvr::vrRenderCommand::EnableDepthTest(tinyvr::vrAPIOPDepth::DEPTH_LESS, false);
			tinyvr::vrRenderCommand::EnableAlphaBlend(tinyvr::vrAPIOPBlendSrc::SRCBLEND_ONE, tinyvr::vrAPIOPBlendDst::DSTBLEND_ONE, 0);
			tinyvr::vrRenderCommand::EnableAlphaBlend(tinyvr::vrAPIOPBlendSrc::SRCBLEND_ZERO, tinyvr::vrAPIOPBlendDst::DSTBLEND_ONE_MINUS_SRCCOLOR, 1);

			tinyvr::vrRenderCommand::ClearBuffer(tinyvr::vrAPIOPBufferType::BUFFERTYPE_COLOR, 0, zeroFillerVec);
			tinyvr::vrRenderCommand::ClearBuffer(tinyvr::vrAPIOPBufferType::BUFFERTYPE_COLOR, 1, oneFillerVec);

			lightingShader->Bind();

			lightingShader->SetFloat3("light.position", lightPos);
			lightingShader->SetFloat3("viewPos", m_CameraController->GetCamera().GetPosition());

			glm::vec3 lightColor = glm::vec3(1.0f);
			glm::vec3 diffuseColor = lightColor * glm::vec3(0.8f);
			glm::vec3 ambientColor = diffuseColor * glm::vec3(0.8f);

			lightingShader->SetFloat3("light.ambient", ambientColor);
			lightingShader->SetFloat3("light.diffuse", diffuseColor);
			lightingShader->SetFloat3("light.specular", glm::vec3(1.0f));

			lightingShader->SetFloat3("material.ambient", { 0.24725f, 0.1995f, 0.0745f });
			lightingShader->SetFloat3("material.diffuse", { 0.75164f, 0.60648f, 0.22648f });
			lightingShader->SetFloat3("material.specular", { 0.628281f, 0.555802f, 0.366065f });
			lightingShader->SetFloat("material.shininess", 0.4f);

			glm::mat4 model = m_TrackBall->getDragMat() * m_ModelMat;
			model = glm::rotate(model, 5.0f * ts.GetTimeEpoch(), glm::vec3(0.0f, 0.0f, 1.0f));
			lightingShader->SetMat4("model", model);

			lightingShader->SetMat4("projection", m_CameraController->GetCamera().GetProjectionMatrix());
			lightingShader->SetMat4("view", m_CameraController->GetCamera().GetViewMatrix());
			lightingShader->SetFloat("opaque", m_Opacity);

			myModel.Draw(*std::dynamic_pointer_cast<tinyvr::vrOpenGLShader>(lightingShader));

			transparentFBO->Unbind();
		}

		{
			tinyvr::vrRenderCommand::EnableDepthTest(tinyvr::vrAPIOPDepth::DEPTH_ALWAYS, false);
			tinyvr::vrRenderCommand::EnableAlphaBlend(
				tinyvr::vrAPIOPBlendSrc::SRCBLEND_SRCALPHA,
				tinyvr::vrAPIOPBlendDst::DSTBLEND_ONE_MINUS_SRCALPHA);

			opaqueFBO->Bind();

			accumTexture->BindUnit(0);
			revealTexture->BindUnit(1);

			tinyvr::vrRenderer::Submit(compositeShader, quadVA);

			opaqueFBO->Unbind();
		}

		{
			tinyvr::vrRenderCommand::EnableDepthTest(tinyvr::vrAPIOPDepth::DEPTH_LESS);
			tinyvr::vrRenderCommand::DisableDepthTest();
			tinyvr::vrRenderCommand::DisableAlphaBlend();

			tinyvr::vrRenderCommand::SetClearColor(glm::vec4(0.0f));
			tinyvr::vrRenderCommand::Clear(true);

			opaqueTexture->BindUnit(0);

			tinyvr::vrRenderer::Submit(screenShader, quadVA);
		}

		tinyvr::vrRenderer::EndScene();
	}

	void IOTLayer::OnImGuiRender()
	{
		ImGui::Begin("Settings");
		ImGui::ColorEdit4("Color", glm::value_ptr(m_BGColor));
		ImGui::Separator();
		ImGui::SliderFloat("Opacity", &m_Opacity, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
		ImGui::End();
	}

	void IOTLayer::OnEvent(tinyvr::vrEvent& e)
	{
		m_CameraController->OnEvent(e);
	}
}