#include "vrRCLayer.h"

#include "imgui/imgui.h"

#include "platform/openGL/vrOpenGLShader.h"
#include "platform/openGL/vrOpenGLErrorHandle.h"

#include "tinyvr/renderer/vrTexture.h"

#include "../files/datFile.h"

namespace rc {

	vrRCLayer::vrRCLayer(const std::string& datFilePath)
		: vrLayer("Ray-Casting Layer"),
		m_ScrSize(1280, 720), m_ModelMat(1.0f), m_VolumeValSpan(0.0f, 0.0f),
		m_ViewportFocused(false), m_ViewportHovered(false), m_RotateObject(true),
		m_ViewportSize(m_ScrSize.x, m_ScrSize.y)
	{
		{
			m_CameraController.reset(new tinyvr::vrPerspectiveCameraController(
				{ 0.0f, 0.0f, 2.0f }, m_ScrSize.x, m_ScrSize.y));
			// member variables initialization
			m_BGColor = glm::vec4(82.0f, 87.0f, 110.0f, 255.0f) / glm::vec4(255.0f);
			m_TrackBall.reset(new tinyvr::vrTrackBall_CHEN(m_MousePos));
		}

		//vertex array of cube
		{
			float cubeVerts[] =
			{
				-0.5, -0.5, -0.5, 0.0, 0.0, 0.0,
				 0.5, -0.5, -0.5, 1.0, 0.0, 0.0,
				 0.5,  0.5, -0.5, 1.0, 1.0, 0.0,
				-0.5,  0.5, -0.5, 0.0, 1.0, 0.0,
				-0.5, -0.5,  0.5, 0.0, 0.0, 1.0,
				 0.5, -0.5,  0.5, 1.0, 0.0, 1.0,
				 0.5,  0.5,  0.5, 1.0, 1.0, 1.0,
				-0.5,  0.5,  0.5, 0.0, 1.0, 1.0
			};

			// Cube Vertex Buffer
			tinyvr::vrRef<tinyvr::vrVertexBuffer> cubeVB;
			cubeVB = tinyvr::vrVertexBuffer::Create(cubeVerts, sizeof(cubeVerts));

			m_CubeVA = tinyvr::vrVertexArray::Create();
			cubeVB->SetLayout({
				{ tinyvr::ShaderDataType::Float3, "a_Position" },
				{ tinyvr::ShaderDataType::Float3, "a_TexCoord" }
				});
			m_CubeVA->AddVertexBuffer(cubeVB);

			// Cube Indices Buffer
			uint32_t cubeIndices[] = {
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
			tinyvr::vrRef<tinyvr::vrIndexBuffer> cubeIB;
			cubeIB = tinyvr::vrIndexBuffer::Create(cubeIndices, sizeof(cubeIndices) / sizeof(uint32_t));
			m_CubeVA->SetIndexBuffer(cubeIB);
		}

		{
			float screenVerts[] =
			{
				 1.0f,  1.0f, 1.0f, 1.0f,
				-1.0f,  1.0f, 0.0f, 1.0f,
				-1.0f, -1.0f, 0.0f, 0.0f,
				 1.0f, -1.0f, 1.0f, 0.0f
			};

			// Screen Vertex Buffer
			tinyvr::vrRef<tinyvr::vrVertexBuffer> screenVB;
			screenVB = tinyvr::vrVertexBuffer::Create(screenVerts, sizeof(screenVerts));

			m_ScreenVA = tinyvr::vrVertexArray::Create();
			screenVB->SetLayout({
				{ tinyvr::ShaderDataType::Float2, "a_Position" },
				{ tinyvr::ShaderDataType::Float2, "a_TexCoord" }
				});
			m_ScreenVA->AddVertexBuffer(screenVB);

			// Screen Indices Buffer
			uint32_t screenIndices[] = {
				0, 1, 2,
				2, 3, 0
			};
			tinyvr::vrRef<tinyvr::vrIndexBuffer> screenIB;
			screenIB = tinyvr::vrIndexBuffer::Create(screenIndices, sizeof(screenIndices) / sizeof(uint32_t));
			m_ScreenVA->SetIndexBuffer(screenIB);
		} 
		
		// Shader here~
		{
			m_ProjShader = tinyvr::vrShader::Create("resources/shaders/rayCasting/rayProj_vert.glsl", "resources/shaders/rayCasting/rayProj_frag.glsl");
			m_RCShader = tinyvr::vrShader::Create("resources/shaders/rayCasting/rayCastingFlat_vert.glsl", "resources/shaders/rayCasting/rayCastingFlat_frag.glsl");
		}

		// raw file here
		{
			file::datFile file(datFilePath);
			if (!file.Read())
			{
				std::cout << "Read file " << datFilePath << " Failed\n";
				return;
			}

			std::cout << file;

			m_ModelMat = file.GetModelMat();
			m_VolumeValSpan = file.ValueSpan();

			auto volBuffer = file.GetData();
			m_DataRes = file.GetResolution();

			m_DataHistogram.resize(256, 0.0);
			float dv = (m_VolumeValSpan.y - m_VolumeValSpan.x + 1.0f) / 256;
			for (auto e : volBuffer)
			{
				int idx = std::ceil((e - m_VolumeValSpan.x) / dv);
				m_DataHistogram[idx] += 1.0f;
			}
			uint32_t NSamples = m_DataRes.x * m_DataRes.y * m_DataRes.z;
			std::transform(m_DataHistogram.begin(), m_DataHistogram.end(), m_DataHistogram.begin(), [&](float v) { return v / (NSamples); });

			auto mM = std::minmax_element(m_DataHistogram.begin(), m_DataHistogram.end());
			m_HistRange.x = *(mM.first);
			m_HistRange.y = *(mM.second);

			m_VolumeTex = tinyvr::vrTexture3D::Create(m_DataRes.x, m_DataRes.y, m_DataRes.z, tinyvr::vrTextureFormat::TEXTURE_FMT_RED);

			m_VolumeTex->SetData(NSamples, volBuffer.data());
		}

		{
			m_FrameSpec.Width = m_ScrSize.x;
			m_FrameSpec.Height = m_ScrSize.y;

			m_ProjInFB = tinyvr::vrFrameBuffer::Create(m_FrameSpec);
			m_ProjInFB->CheckStatus();

			m_ProjOutFB = tinyvr::vrFrameBuffer::Create(m_FrameSpec);
			m_ProjOutFB->CheckStatus();

			m_RenderFB = tinyvr::vrFrameBuffer::Create(m_FrameSpec);
			m_RenderFB->CheckStatus();
		}

		m_TransferFunction = tinyvr::vrTransferFunction::Create(m_TFFuncTex);
		m_TFWidget = tinyvr::vrTransferFunctionWidget::Create(m_TransferFunction, "RayCasting");
	}
	
	void vrRCLayer::OnUpdate(tinyvr::vrTimestep ts)
	{
		m_TimeStep = ts;
		{
			if (auto spec = m_RenderFB->GetSpecification();
				m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f &&	// zero sized framebuffer is invalid
				(spec.Width != m_ViewportSize.x || spec.Height != m_ViewportSize.y))
			{
				m_CameraController->OnResize(m_ViewportSize.x, m_ViewportSize.y);
				tinyvr::vrRenderCommand::SetViewPort(0, 0, m_ViewportSize.x, m_ViewportSize.y);

				m_ProjInFB->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
				m_ProjOutFB->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
				m_RenderFB->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
			}
		}

		{
			if(m_ViewportHovered)
				if (!m_TrackBall->IsMousePressed() && tinyvr::vrInput::IsMouseButtonPressed(TINYVR_MOUSE_BUTTON_LEFT))
					m_TrackBall->mousePressed();

			if (m_TrackBall->IsMousePressed())
				if (m_ViewportFocused)
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
		}

		tinyvr::vrRenderCommand::DisableAlphaBlend();
		tinyvr::vrRenderCommand::SetClearColor({ 0.0f, 0.0f, 0.0f, 1.0f });

		if (m_RotateObject)
			m_ModelMat = glm::rotate(glm::mat4(1.0f), glm::radians(50.0f * (float)ts), { 0.0f, 1.0f, 0.0f }) * m_ModelMat;
		tinyvr::vrRenderer::BeginScene(m_CameraController->GetCamera());

		/// ============ Render Begin ============

		// render to texture, proj coord in
		{
			m_ProjShader->Bind();
			m_ProjShader->SetMat4("u_Model", m_ModelMat);

			m_ProjInFB->Bind();

			tinyvr::vrRenderCommand::EnableDepthTest(tinyvr::vrAPIOPDepth::DEPTH_LESS);
			tinyvr::vrRenderCommand::EnableCullFace(tinyvr::vrAPIOPFaceCull::CULLFACE_BACK);

			tinyvr::vrRenderCommand::SetClearDepth(1.0f);
			tinyvr::vrRenderCommand::Clear();

			tinyvr::vrRenderer::Submit(m_ProjShader, m_CubeVA, m_TrackBall->getDragMat());

			m_ProjInFB->Unbind();
		}

		// render to texture, proj coord out
		{
			m_ProjOutFB->Bind();

			tinyvr::vrRenderCommand::EnableDepthTest(tinyvr::vrAPIOPDepth::DEPTH_GREATER);
			tinyvr::vrRenderCommand::EnableCullFace(tinyvr::vrAPIOPFaceCull::CULLFACE_FRONT);

			tinyvr::vrRenderCommand::SetClearDepth(0.0f);
			tinyvr::vrRenderCommand::Clear();

			tinyvr::vrRenderer::Submit(m_ProjShader, m_CubeVA, m_TrackBall->getDragMat());

			m_ProjOutFB->Unbind();
		}

		// render to screen, ray casting procedure
		{
			m_RenderFB->Bind();

			tinyvr::vrRenderCommand::EnableDepthTest(tinyvr::vrAPIOPDepth::DEPTH_LESS);
			tinyvr::vrRenderCommand::EnableCullFace(tinyvr::vrAPIOPFaceCull::CULLFACE_BACK);
			tinyvr::vrRenderCommand::EnableAlphaBlend(
				tinyvr::vrAPIOPBlendSrc::SRCBLEND_SRCALPHA,
				tinyvr::vrAPIOPBlendDst::DSTBLEND_ONE_MINUS_SRCALPHA);

			tinyvr::vrRenderCommand::SetClearColor(m_BGColor);
			tinyvr::vrRenderCommand::SetClearDepth(1.0);
			tinyvr::vrRenderCommand::Clear();

			m_RCShader->Bind();

			m_RCShader->SetFloat("v_Min", m_VolumeValSpan.x);
			m_RCShader->SetFloat("v_Max", m_VolumeValSpan.y);

			m_RCShader->SetTexture("t_CoordIn", m_ProjInFB->GetColorAttachment());
			m_RCShader->SetTexture("t_CoordOut", m_ProjOutFB->GetColorAttachment());

			m_RCShader->SetTexture("t_Volume", m_VolumeTex);
			m_RCShader->SetTexture("t_TFFunc", m_TFFuncTex);

			m_RCShader->SetInt3("t_DIM", m_DataRes);
			
			tinyvr::vrRenderer::Submit(m_RCShader, m_ScreenVA);

			m_RenderFB->Unbind();
		}

		/// ============ Render End ============
		tinyvr::vrRenderer::EndScene();
	}

	void vrRCLayer::OnEvent(tinyvr::vrEvent& e)
	{
		m_CameraController->OnEvent(e);
	}

	void rc::vrRCLayer::OnImGuiRender()
	{
		static bool dockspaceOpen = true;
		static bool opt_fullscreen_persistant = true;
		bool opt_fullscreen = opt_fullscreen_persistant;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (opt_fullscreen)
		{
			ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->Pos);
			ImGui::SetNextWindowSize(viewport->Size);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
			window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}

		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("DockSpace Demo", &dockspaceOpen, window_flags);
		ImGui::PopStyleVar();

		if (opt_fullscreen)	ImGui::PopStyleVar(2);

		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Exit")) tinyvr::vrApplication::Get().Close();
				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		{
			ImGui::Begin("Settings");

			ImGui::ColorEdit3("Background Color", glm::value_ptr(m_BGColor));
			ImGui::Checkbox("Rotate Object", &m_RotateObject);
			ImGui::SameLine();
			ImGui::Text("FPS : %6.2f", 1.0f / m_TimeStep);

			ImGui::End();
		}

		m_TFWidget->draw(m_DataHistogram, m_HistRange);
		
		{
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0.0f, 0.0f });
			ImGui::Begin("Viewport");

			{
				m_ViewportFocused = ImGui::IsWindowFocused();
				m_ViewportHovered = ImGui::IsWindowHovered();
				tinyvr::vrApplication::Get().GetImGuiLayer()->BlockEvents(!m_ViewportFocused || !m_ViewportHovered);

				ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
				if (m_ViewportSize != glm::vec2(viewportPanelSize.x, viewportPanelSize.y)
					&& viewportPanelSize.x > 0 && m_ViewportSize.y > 0)
				{
					m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };
					m_CameraController->OnResize(viewportPanelSize.x, viewportPanelSize.y);
				}
			}

			ImGui::Image((void*)m_RenderFB->GetColorAttachmentRendererID(), ImVec2{ m_ViewportSize.x, m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

			ImGui::End();
			ImGui::PopStyleVar();
		}

		ImGui::End();
	}
}