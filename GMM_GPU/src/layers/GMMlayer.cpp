#include "GMMlayer.h"
#include <imgui/imgui.h>

#include "../files/GMMFile.h"
#include "../utils/EM_Util.h"

#include "tinyVR/renderer/vrShader.h"
#include "platform/openGL/vrOpenGLCompShader.h"

#include "platform/openGL/vrOpenGLErrorHandle.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>

#include "../files/lodepng.h"

bool encodePNG(const std::filesystem::path& filePath, const unsigned char* rawImageData, uint32_t width, uint32_t height)
{
	// Encode the image
	unsigned error = lodepng_encode32_file(filePath.string().c_str(), rawImageData, width, height);
	TINYVR_ASSERT(!error, "ERROR {0:>12}: {1}", error, lodepng_error_text(error));
	return error;

}

namespace gmm {

	uint32_t maxOctreeDepth = 0;
	constexpr uint32_t patchSize = 4;		// for grid refinement
	constexpr uint32_t stencilSize = 3;		// for localEntropy

	vrGMMLayer::vrGMMLayer(
		const std::string& gmmDataDir, glm::uvec3 brickRes,
		uint32_t blockSize, glm::uvec3 dataRes,
		glm::vec2 valueRange, uint32_t numIntervals) :
			m_GMMBaseDir(gmmDataDir), m_BrickRes(brickRes), m_BlockSize(blockSize),
			m_DataRes(dataRes), m_ValueRange(valueRange), m_NumIntervals(numIntervals),
			m_ScrSize(1280, 720), m_ModelMat(1.0f), m_BGColor(0.32156862f, 0.34117647f, 0.43137255f, 1.0f),
			m_MousePos(0.0f), m_ViewportSize(m_ScrSize.x, m_ScrSize.y), m_GAMMA(0.3f), m_EntropyRange(0.0f)
	{
		m_UserDefinedMaxTreeDepth = 0;

		m_GridColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		m_VisVolOrEntropy = true;
		m_ShowGrid = true;
		m_ShowPatch = false;
		m_ClipVolume = false;
		m_ClipPlane = 0.0f;

		m_timeStep = 1.0f;
		m_ViewportFocused = m_ViewportHovered = false;

		initParam();
		initVolumeTex();
		
		initGMMFile();

		//CorrGMMFile_CPU();
		//m_GMMFile->WriteToDir("D://SGMMData_Lap3D");

		reconVol();
		calculateHistogram();

		//storeVolumeDataAsFile();

		initRCComponent();

		// calculate entropy
		initEntropyComponent();

		constructOctree();
		// Reconstruction Quality
		reconByAMR();
		measureRMSE();
		//measureSSIM();
	}

	vrGMMLayer::~vrGMMLayer()
	{
		delete[] m_XGap;
		delete[] m_YGap;
		delete[] m_ZGap;

		delete[] m_XBlockGap;
		delete[] m_YBlockGap;
		delete[] m_ZBlockGap;
	}

	void vrGMMLayer::OnUpdate(tinyvr::vrTimestep ts)
	{
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
				m_DisplayFB->Resize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
			}
		}

		{
			m_timeStep = ts;
			if (m_ViewportHovered)
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
					else	m_TrackBall->mouseMove();
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

		/// ============ Render Begin ============
		//m_ModelMat = glm::rotate(glm::mat4(1.0f), glm::radians(1.0f), { 0.0f, 1.0f, 0.0f }) * m_ModelMat;
		tinyvr::vrRenderer::BeginScene(m_CameraController->GetCamera());

		m_ProjShader->Bind();
		m_ProjShader->SetMat4("u_Model", m_ModelMat);
		
		{	// render to texture, proj coord in
			m_ProjInFB->Bind();

			tinyvr::vrRenderCommand::EnableDepthTest(tinyvr::vrAPIOPDepth::DEPTH_LESS);
			tinyvr::vrRenderCommand::EnableCullFace(tinyvr::vrAPIOPFaceCull::CULLFACE_BACK);

			tinyvr::vrRenderCommand::SetClearDepth(1.0f);
			tinyvr::vrRenderCommand::Clear();

			tinyvr::vrRenderer::Submit(m_ProjShader, m_CubeVA, m_TrackBall->getDragMat());

			m_ProjInFB->Unbind();
		}

		{	// render to texture, proj coord out
			m_ProjOutFB->Bind();

			tinyvr::vrRenderCommand::EnableDepthTest(tinyvr::vrAPIOPDepth::DEPTH_GREATER);
			tinyvr::vrRenderCommand::EnableCullFace(tinyvr::vrAPIOPFaceCull::CULLFACE_FRONT);

			tinyvr::vrRenderCommand::SetClearDepth(0.0f);
			tinyvr::vrRenderCommand::Clear();

			tinyvr::vrRenderer::Submit(m_ProjShader, m_CubeVA, m_TrackBall->getDragMat());

			m_ProjOutFB->Unbind();
		}
		
		{	// render to screen, tree-based ray-casting procedure
			m_RenderFB->Bind();

			tinyvr::vrRenderCommand::EnableDepthTest(tinyvr::vrAPIOPDepth::DEPTH_LESS);
			tinyvr::vrRenderCommand::EnableCullFace(tinyvr::vrAPIOPFaceCull::CULLFACE_BACK);
			tinyvr::vrRenderCommand::EnableAlphaBlend(
				tinyvr::vrAPIOPBlendSrc::SRCBLEND_SRCALPHA,
				tinyvr::vrAPIOPBlendDst::DSTBLEND_ONE_MINUS_SRCALPHA);

			tinyvr::vrRenderCommand::SetClearColor(glm::vec4(0.0f));
			tinyvr::vrRenderCommand::SetClearDepth(1.0);
			tinyvr::vrRenderCommand::Clear();

			m_AMRRCShader->Bind();

			m_AMRRCShader->SetMat4("u_Model", m_ModelMat);

			m_AMRRCShader->SetFloat("SCR_WIDTH", m_ViewportSize.x);
			m_AMRRCShader->SetFloat("SCR_HEIGHT", m_ViewportSize.y);

			if (m_VisVolOrEntropy) {
				m_AMRRCShader->SetTexture("renderVolume", m_VolumeTex);
				m_AMRRCShader->SetTexture("tFunc", m_TFVolume->GetTFTexture());

				m_AMRRCShader->SetFloat("vMin", m_ValueRange.x);
				m_AMRRCShader->SetFloat("vMax", m_ValueRange.y);
			}
			else {
				m_AMRRCShader->SetTexture("renderVolume", m_LocalEntropyTex);
				m_AMRRCShader->SetTexture("tFunc", m_TFEntropy->GetTFTexture());

				m_AMRRCShader->SetFloat("vMin", m_EntropyRange.x);
				m_AMRRCShader->SetFloat("vMax", m_EntropyRange.y);
			}

			m_AMRRCShader->SetTexture("octreeNodePool", m_TreeNodeTex);
			m_AMRRCShader->SetTexture("octreeNodePos", m_TreePosTex);

			m_AMRRCShader->SetTexture("coordIn", m_ProjInFB->GetColorAttachment());
			m_AMRRCShader->SetTexture("coordOut", m_ProjOutFB->GetColorAttachment());

			m_AMRRCShader->SetInt("maxOctreeDepth", treeMaxDepth);
			m_AMRRCShader->SetInt("patchSize", patchSize);
			m_AMRRCShader->SetInt("UserDefinedDepth", m_UserDefinedMaxTreeDepth);

			m_AMRRCShader->SetBool("showGrid", m_ShowGrid);
			m_AMRRCShader->SetBool("showPatch", m_ShowPatch);
			m_AMRRCShader->SetBool("clipVolume", m_ClipVolume);
			m_AMRRCShader->SetFloat("clipPlane", m_ClipPlane);

			m_AMRRCShader->SetFloat("GAMMA", m_GAMMA);
			m_AMRRCShader->SetFloat4("GColor", m_GridColor);

			tinyvr::vrRenderer::Submit(m_AMRRCShader, m_CubeVA, m_TrackBall->getDragMat());

			m_RenderFB->Unbind();
		}

		{	// render to screen, texture
			m_DisplayFB->Bind();

			tinyvr::vrRenderCommand::EnableDepthTest(tinyvr::vrAPIOPDepth::DEPTH_LESS);
			tinyvr::vrRenderCommand::EnableCullFace(tinyvr::vrAPIOPFaceCull::CULLFACE_BACK);
			tinyvr::vrRenderCommand::EnableAlphaBlend(
				tinyvr::vrAPIOPBlendSrc::SRCBLEND_SRCALPHA,
				tinyvr::vrAPIOPBlendDst::DSTBLEND_ONE_MINUS_SRCALPHA);

			tinyvr::vrRenderCommand::SetClearColor(m_BGColor);
			tinyvr::vrRenderCommand::SetClearDepth(1.0);
			tinyvr::vrRenderCommand::Clear();

			m_DisplayShader->Bind();
			m_DisplayShader->SetTexture("tex", m_RenderFB->GetColorAttachment(0));

			tinyvr::vrRenderer::Submit(m_DisplayShader, m_DisplayVA);

			m_DisplayFB->Unbind();
		}

		tinyvr::vrRenderer::EndScene();
	}

	void vrGMMLayer::OnEvent(tinyvr::vrEvent& e)
	{
		m_CameraController->OnEvent(e);
	}

	void vrGMMLayer::OnImGuiRender()
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
			ImGui::Separator();

			ImGui::Text("Data Resolution : [%4d, %4d, %4d]", m_DataRes.x, m_DataRes.y, m_DataRes.z);
			ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

			ImGui::Separator();

			ImGui::Text("Render duration : %6.3f ms", 1000.0 * m_timeStep);
			ImGui::Text("FPS = %3d", static_cast<int>(1.0f / m_timeStep));
			ImGui::Separator();
			ImGui::SliderFloat("Gamma Coorection", &m_GAMMA, 0.0f, 1.0f, "ratio=%.3f", ImGuiSliderFlags_Logarithmic);

			ImGui::Text("Control Panel");
			ImGui::Checkbox("showGrid", &m_ShowGrid);
			ImGui::SameLine();
			ImGui::Checkbox("showPatch", &m_ShowPatch);
			ImGui::Checkbox("clipVolume", &m_ClipVolume);
			ImGui::SliderFloat("clip plane", &m_ClipPlane, 0.0f, 1.0f, "ratio=%.2f", ImGuiSliderFlags_None);
			ImGui::Checkbox("showVolume", &m_VisVolOrEntropy);
			ImGui::SliderInt("RenderDepth", &m_UserDefinedMaxTreeDepth, 0, maxOctreeDepth, "%01d");
			ImGui::ColorEdit4("Grid Line Color", glm::value_ptr(m_GridColor));

			ImGui::Separator();
			ImGui::RadioButton("Opaque", &m_SaveOpaqueOrNot, 0);
			ImGui::SameLine();
			ImGui::RadioButton("With BG", &m_SaveOpaqueOrNot, 1);

			if (ImGui::Button("Save Rendered Image As..."))
			{
				tinyvr::vrRef<tinyvr::vrTexture2D> tex;
				if (m_SaveOpaqueOrNot == 0) tex = m_RenderFB->GetColorAttachment(0);
				else if (m_SaveOpaqueOrNot == 1) tex = m_DisplayFB->GetColorAttachment(0);
				
				auto W = tex->GetWidth();
				auto H = tex->GetHeight();
				std::vector<glm::vec4> imageBuffer(W * H);
				tex->GetData(imageBuffer.data());

				std::vector<glm::u8vec4> imageBufferUChar(W * H);
				std::transform(imageBuffer.begin(), imageBuffer.end(), imageBufferUChar.begin(), 
					[&](const glm::vec4& pixel)
					{
						uint8_t r = static_cast<uint8_t>(pixel.r * 255.999f);
						uint8_t g = static_cast<uint8_t>(pixel.g * 255.999f);
						uint8_t b = static_cast<uint8_t>(pixel.b * 255.999f);
						uint8_t a = static_cast<uint8_t>(pixel.a * 255.999f);
						return glm::u8vec4(r, g, b, a);
					});

				if (m_SaveOpaqueOrNot == 0)	encodePNG("S:/SGMM Pic/a_opaque.png", (const unsigned char*)imageBufferUChar.data(), W, H);
				else if (m_SaveOpaqueOrNot == 1) encodePNG("S:/SGMM Pic/a_withBG.png", (const unsigned char*)imageBufferUChar.data(), W, H);
			}

			ImGui::End();
		}

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

			ImGui::Image((void*)m_DisplayFB->GetColorAttachmentRendererID(), ImVec2{ m_ViewportSize.x, m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

			m_TFVolumeWidget->draw(m_VolumeHistogram, m_VolumeHistRange);
			m_TFEntropyWidget->draw(m_EntopyHistogram, m_EntropyHistRange);

			ImGui::End();
			ImGui::PopStyleVar();
		}

		ImGui::End();
	}

	void vrGMMLayer::initParam()
	{
		{
			m_XGap = new int[m_BrickRes.x + 1];
			m_YGap = new int[m_BrickRes.y + 1];
			m_ZGap = new int[m_BrickRes.z + 1];

			int dx = m_DataRes.x / m_BrickRes.x;
			int dy = m_DataRes.y / m_BrickRes.y;
			int dz = m_DataRes.z / m_BrickRes.z;

			m_XGap[0] = 0;
			m_XGap[m_BrickRes.x] = m_DataRes.x;
			for (int i = 1; i < m_BrickRes.x; ++i)	m_XGap[i] = i * dx;

			m_YGap[0] = 0;
			m_YGap[m_BrickRes.y] = m_DataRes.y;
			for (int j = 1; j < m_BrickRes.y; ++j)	m_YGap[j] = j * dy;

			m_ZGap[0] = 0;
			m_ZGap[m_BrickRes.z] = m_DataRes.z;
			for (int k = 1; k < m_BrickRes.z; ++k)	m_ZGap[k] = k * dz;
		}

		{
			m_XBlockGap = new int[m_BrickRes.x + 1];
			m_YBlockGap = new int[m_BrickRes.y + 1];
			m_ZBlockGap = new int[m_BrickRes.z + 1];

			m_XBlockGap[0] = 0;
			for (int i = 1; i < m_BrickRes.x + 1; ++i)
				m_XBlockGap[i] = (m_XGap[i] - m_XGap[i - 1]) / m_BlockSize + m_XBlockGap[i - 1];
			
			m_YBlockGap[0] = 0;
			for (int j = 1; j < m_BrickRes.y + 1; ++j)
				m_YBlockGap[j] = (m_YGap[j] - m_YGap[j - 1]) / m_BlockSize + m_YBlockGap[j - 1];

			m_ZBlockGap[0] = 0;
			for (int k = 1; k < m_BrickRes.z + 1; ++k)
				m_ZBlockGap[k] = (m_ZGap[k] - m_ZGap[k - 1]) / m_BlockSize + m_ZBlockGap[k - 1];
		}
	}

	void vrGMMLayer::initGMMFile()
	{
		TINYVR_PROFILE_FUNCTION();

		uint32_t numBricks = m_BrickRes.x * m_BrickRes.y * m_BrickRes.z;
		if (m_GMMFile == nullptr)
		{
			m_GMMFile = tinyvr::CreateRef<gmm::GMMFile>(m_BlockSize, m_NumIntervals, numBricks);

			TINYVR_ASSERT(m_GMMFile->read(m_GMMBaseDir), "Read data Failed");
			TINYVR_INFO("Load data success!");
		}

		uint32_t KernelNum = m_GMMFile->GetmaxKernelNum();	// 4, probabily

		// compare data buffer, storing the reordered data
		// | ---- | ---- | ---- | ---- | X 24 | ---- | ---- | ---- | ---- | X 24
		//   binW   米k_x   米k_y   米k_z          kerW   考k_x   考k_y   考k_z
		std::vector<glm::vec4> GMMCoeffBuffer;

		unsigned int W = m_GMMFile->GetmaxBlockNum();
		unsigned int D = m_NumIntervals;			// 256
		// [0 .. 23] : binW & 米k_[1, 2, 3], [24 .. 47] : kerW & 考k_[1, 2, 3]
		unsigned int H = numBricks * 2;				// 24 * 2
		auto L = W * H * D;

		for (unsigned int n = 0; n < KernelNum; ++n)
		{
			TINYVR_INFO("Prepare kernel {0}", n + 1);

			GMMCoeffBuffer.clear();
			GMMCoeffBuffer.resize(1ULL * L, glm::vec4(1.0f, 1.0f, 1.0f, 0.0f));

			m_GMMCoeffTexturesList.push_back(tinyvr::vrTexture3D::Create(W, D, H));

			for (int k = 0; k < numBricks; ++k)
			{
				const auto& info = (*m_GMMFile)[k].first;
				const auto& data = (*m_GMMFile)[k].second;

				int I = std::min(info.numBlocks, W);
				for (int i = 0; i < I; ++i)
				{
					const auto& GMMBinList = data.GMMListPerBlock[i];
					for (int j = 0; j < D; ++j)
					{
						const auto& bin = GMMBinList[j];
						if (n >= bin.GetKernelNum())	continue;

						const auto& kernel = bin.GetKernel(n);

						// GMMBin weight & mean value
						auto idx = k * (W * D) + j * W + i;
						auto& texelForMean = GMMCoeffBuffer[idx];

						texelForMean.w = bin.GetProb();
						texelForMean.x = kernel.funcs.x.mean;
						texelForMean.y = kernel.funcs.y.mean;
						texelForMean.z = kernel.funcs.z.mean;

						// Kernel weight & var value
						idx += numBricks * (W * D);
						auto& texelForVar = GMMCoeffBuffer[idx];

						texelForVar.w = kernel.weight;
						texelForVar.x = kernel.funcs.x.var;
						texelForVar.y = kernel.funcs.y.var;
						texelForVar.z = kernel.funcs.z.var;
					}
				}
			}
			m_GMMCoeffTexturesList.back()->SetData(W * D * H, GMMCoeffBuffer.data());
		}
	}

	void vrGMMLayer::initVolumeTex()
	{
		TINYVR_PROFILE_FUNCTION();

		m_VolumeTex = tinyvr::vrTexture3D::Create(m_DataRes.x, m_DataRes.y, m_DataRes.z,
			tinyvr::vrTextureFormat::TEXTURE_FMT_RED, tinyvr::vrTextureType::TEXTURE_TYPE_FLT32);
		m_VolumeTex->SetData(m_DataRes.x * m_DataRes.y * m_DataRes.z);

		m_CopyVolumeTex = tinyvr::vrTexture3D::Create(m_DataRes.x, m_DataRes.y, m_DataRes.z,
			tinyvr::vrTextureFormat::TEXTURE_FMT_RED, tinyvr::vrTextureType::TEXTURE_TYPE_FLT32);
		m_CopyVolumeTex->SetData(m_DataRes.x * m_DataRes.y * m_DataRes.z);

		m_OriginVolumeTex = tinyvr::vrTexture3D::Create(m_DataRes.x, m_DataRes.y, m_DataRes.z,
			tinyvr::vrTextureFormat::TEXTURE_FMT_RED, tinyvr::vrTextureType::TEXTURE_TYPE_FLT32);

		std::vector<uint8_t> originData(m_DataRes.x * m_DataRes.y * m_DataRes.z, 0x00);
		m_OriginDataBuffer.resize(m_DataRes.x * m_DataRes.y * m_DataRes.z, 0.0f);
		
		//std::ifstream originFile("resources/data/alter/alter-output-8-1-sgmm/unsignedcharVolume.raw", std::ios::binary);
		//std::ifstream originFile("resources/data/stone/stone-output-8-1-stone-sgmm/unsignedcharVolume.raw", std::ios::binary);
		//std::ifstream originFile("resources/data/stone/stone-output-8-1-stone2-sgmm/unsignedcharVolume.raw", std::ios::binary);
		std::ifstream originFile("resources/data/lap3d/lap3d-output-8-1-lap3d-sgmm/unsignedcharVolume.raw", std::ios::binary);
		//std::ifstream originFile("resources/data/shock/shock-output-8-1-shock-sgmm/unsignedcharVolume.raw", std::ios::binary);
		//std::ifstream originFile("resources/data/plane/plane-output-8-1-plane-sgmm/unsignedcharVolume.raw", std::ios::binary);
		TINYVR_ASSERT(originFile, "Open File Failed!");
		originFile.read((char*)originData.data(), m_DataRes.x * m_DataRes.y * m_DataRes.z);
		std::copy(originData.begin(), originData.end(), m_OriginDataBuffer.begin());
		originFile.close();

		//std::ifstream originFile("S:/SGMM Data/snd/unsignedcharVolume.raw", std::ios::binary);
		//originFile.read((char*)m_OriginDataBuffer.data(), m_DataRes.x * m_DataRes.y * m_DataRes.z * sizeof(float));
		//originFile.close();

		m_OriginVolumeTex->SetData(m_DataRes.x * m_DataRes.y * m_DataRes.z, m_OriginDataBuffer.data());
	}

	void vrGMMLayer::reconVol()
	{
		TINYVR_PROFILE_FUNCTION();

		if (m_ReconCompShader == nullptr)
			m_ReconCompShader = tinyvr::vrShader::CreateComp("resources/shaders/compute/reconstruct3DVolume_comp.glsl");
		m_ReconCompShader->Bind();

		m_VolumeTex->BindImage(0, tinyvr::vrTexImageAccess::TEXTURE_IMAGE_ACCESS_WRITEONLY);
		for (int i = 0; i < m_GMMCoeffTexturesList.size(); ++i)		m_GMMCoeffTexturesList[i]->BindUnit(i + 1);

		m_ReconCompShader->SetInt("B", m_BlockSize);
		m_ReconCompShader->SetFloat("vMin", m_ValueRange.x);
		m_ReconCompShader->SetFloat("vMax", m_ValueRange.y);
		m_ReconCompShader->SetInt("NumIntervals", m_NumIntervals);
		m_ReconCompShader->SetInt("NumBricks", m_BrickRes.x * m_BrickRes.y * m_BrickRes.z);

		for (int i = 0; i < m_BrickRes.x; ++i)
			for (int j = 0; j < m_BrickRes.y; ++j)
				for (int k = 0; k < m_BrickRes.z; ++k)
				{
					auto O = glm::ivec3(m_XGap[i], m_YGap[j], m_ZGap[k]);
					auto R = glm::ivec3(m_XGap[i + 1], m_YGap[j + 1], m_ZGap[k + 1]) - O;

					m_ReconCompShader->SetInt3("O", O);
					m_ReconCompShader->SetInt3("R", R);

					m_ReconCompShader->SetInt("BRICK_IDX", k * m_BrickRes.y * m_BrickRes.x + j * m_BrickRes.x + i);

					std::dynamic_pointer_cast<tinyvr::vrOpenGLCompShader>(m_ReconCompShader)->Compute(R);
				}

		std::vector<float> fetchBuffer(m_DataRes.x * m_DataRes.y * m_DataRes.z);
		m_VolumeTex->GetData(fetchBuffer.data());

		auto mM = std::minmax_element(fetchBuffer.begin(), fetchBuffer.end());
		glm::vec2 r = { *(mM.first), *(mM.second) };
		
		tinyvr::vrRef<tinyvr::vrShader> copyCompShader = tinyvr::vrShader::CreateComp("resources/shaders/compute/copyTexture_comp.glsl");
		copyCompShader->Bind();

		{	// Draw and calculate on original volume
			//m_VolumeTex->BindImage(0, tinyvr::vrTexImageAccess::TEXTURE_IMAGE_ACCESS_WRITEONLY);
			//m_OriginVolumeTex->BindUnit(1);

			//std::dynamic_pointer_cast<tinyvr::vrOpenGLCompShader>(copyCompShader)->Compute(m_DataRes);
		}

		{
			m_CopyVolumeTex->BindImage(0, tinyvr::vrTexImageAccess::TEXTURE_IMAGE_ACCESS_WRITEONLY);
			m_VolumeTex->BindUnit(1);

			std::dynamic_pointer_cast<tinyvr::vrOpenGLCompShader>(copyCompShader)->Compute(m_DataRes);
		}
	}

	void vrGMMLayer::calculateHistogram()
	{
		m_GlobalHistTex = tinyvr::vrTexture2D::Create(m_NumIntervals, 1,
			tinyvr::vrTextureFormat::TEXTURE_FMT_RED, tinyvr::vrTextureType::TEXTURE_TYPE_U32I);
		m_GlobalHistTex->SetData(m_NumIntervals);

		m_GlobalHistCompShader = tinyvr::vrShader::CreateComp("resources/shaders/compute/calculateGlobalHist_comp.glsl");
		m_GlobalHistCompShader->Bind();

		m_VolumeTex->BindUnit(0);
		m_GlobalHistTex->BindImage(1, tinyvr::vrTexImageAccess::TEXTURE_IMAGE_ACCESS_READWRITE);

		m_GlobalHistCompShader->SetFloat("vMin", m_ValueRange.x);
		m_GlobalHistCompShader->SetFloat("vMax", m_ValueRange.y);
		m_GlobalHistCompShader->SetInt("NumIntervals", m_NumIntervals);

		std::dynamic_pointer_cast<tinyvr::vrOpenGLCompShader>(m_GlobalHistCompShader)->Compute(m_DataRes);

		std::vector<uint32_t> uBuffer = tinyvr::vrFrameBuffer::ReadPixelsUI(m_GlobalHistTex, { 0, 0 }, { m_NumIntervals - 1, 0 });
		m_VolumeHistogram.resize(m_NumIntervals, 0);
		std::transform(uBuffer.begin(), uBuffer.end(), m_VolumeHistogram.begin(), [&](uint32_t v)
			{	return static_cast<float>(v) / (m_DataRes.x * m_DataRes.y * m_DataRes.z); });

		auto mM = std::minmax_element(m_VolumeHistogram.begin(), m_VolumeHistogram.end());
		m_VolumeHistRange = { *(mM.first), *(mM.second) };
	}

	void vrGMMLayer::storeVolumeDataAsFile()
	{
		uint32_t N = m_DataRes.x * m_DataRes.y * m_DataRes.z;
		std::vector<float> reconDataBuffer(N, 0.0f);
		m_VolumeTex->GetData(reconDataBuffer.data());

		std::ofstream file("D:\\corr.raw", std::ios::binary);
		file.write(reinterpret_cast<char*>(reconDataBuffer.data()), sizeof(float) * N);
		file.close();
	}

	struct mutualStruct
	{
		std::array<float, 26> MIs = { 0.0f };
		std::array<float, 26> Hxys = { 0.0f };
		std::array<bool, 26> ExceedVol = { false };
		float binWeightEntropy = 0.0f;
	};

	void vrGMMLayer::CorrGMMFile_CPU()
	{
		uint32_t numBricks = m_BrickRes.x * m_BrickRes.y * m_BrickRes.z;
		if (m_GMMFile == nullptr)
		{
			m_GMMFile = tinyvr::CreateRef<gmm::GMMFile>(m_BlockSize, m_NumIntervals, numBricks);

			TINYVR_ASSERT(m_GMMFile->read(m_GMMBaseDir), "Read data Failed");
			TINYVR_INFO("Load data success!");
		}
		std::cout << *m_GMMFile;

		const std::array<glm::ivec3, 26> Dirs = {{
				// 6-centers
				{  1,  0,  0 }, { -1,  0,  0 },
				{  0,  1,  0 }, {  0, -1,  0 },
				{  0,  0,  1 }, {  0,  0, -1 },
				// 8-vertices
				{  1,  1,  1 }, { -1,  1,  1 },
				{  1, -1,  1 }, { -1, -1,  1 },
				{  1,  1, -1 }, { -1,  1, -1 },
				{  1, -1, -1 }, { -1, -1, -1 },
				// 12-edges
				{  1,  1,  0 }, { -1,  1,  0 },
				{  1, -1,  0 }, { -1, -1,  0 },
				{  1,  0,  1 }, { -1,  0,  1 },
				{  1,  0, -1 }, { -1,  0, -1 },
				{  0,  1,  1 }, {  0, -1,  1 },
				{  0,  1, -1 }, {  0, -1, -1 },
			}
		};

		int numDirs = Dirs.size();

		glm::ivec3 blockRes = { m_XBlockGap[m_BrickRes.x], m_YBlockGap[m_BrickRes.y], m_ZBlockGap[m_BrickRes.z] };
		uint32_t numBlocks = blockRes.x * blockRes.y * blockRes.z;

		// Prepare Basic Variables
		std::vector<std::vector<mutualStruct>> blockNeighborInfoPerBrick(numBricks, std::vector<mutualStruct>());
		std::vector<std::vector<std::vector<float>>> oldBlockBinWeightsPerBrick(
			numBricks, std::vector<std::vector<float>>());

		// Utility Functions
		auto GetCoordFromSeqIdx = [&](int idx, glm::ivec3 res) -> glm::ivec3
		{
			glm::ivec3 c = glm::ivec3(0);
			int i = idx;
			c.z = i / (res.y * res.x);
			i -= c.z * (res.y * res.x);
			c.y = i / res.x;
			i -= c.y * res.x;
			c.x = i;
			return c;
		};

		auto GetSeqIdxFromCoord = [&](glm::ivec3 P, glm::ivec3 res)
		{
			return P.z * res.y * res.x + P.y * res.x + P.x;
		};

		auto CheckExceedRange = [&](glm::ivec3 P, glm::ivec3 res) -> bool
		{
			if (P.x < 0 || P.x >= res.x)	return true;
			if (P.y < 0 || P.y >= res.y)	return true;
			if (P.z < 0 || P.z >= res.z)	return true;
			return false;
		};

		auto SearchWhichBrickInIdx = [&](glm::ivec3 P) ->glm::ivec3
		{
			glm::ivec3 brickCoord(0);
			for (int i = 0; i < m_BrickRes.x; ++i)
				if (P.x >= m_XBlockGap[i] && P.x < m_XBlockGap[i + 1])
					brickCoord.x = i;

			for (int j = 0; j < m_BrickRes.y; ++j)
				if (P.y >= m_YBlockGap[j] && P.y < m_YBlockGap[j + 1])
					brickCoord.y = j;

			for (int k = 0; k < m_BrickRes.z; ++k)
				if (P.z >= m_ZBlockGap[k] && P.z < m_ZBlockGap[k + 1])
					brickCoord.z = k;

			return brickCoord;
		};

		auto CalMInHxy = [&](const std::vector<float>& ws, uint32_t sampN,
			const std::vector<float>& nws, uint32_t nsampN)->std::tuple<float, float>
		{
			float MI = 0.0f;
			float Hxy = 0.0f;
			for (int i = 0; i < m_NumIntervals; ++i)
			{
				float px = ws[i];
				float py = nws[i];
				float pxy = (px * sampN + py * nsampN) / (sampN + nsampN);

				if (px == 0.0f || py == 0.0f)	continue;

				MI += pxy * log(pxy / (px * py));
				Hxy += -pxy * log(pxy);
			}
			MI = std::max(0.0f, MI);
			Hxy = std::max(0.0f, Hxy);
			
			return std::make_tuple(MI, Hxy);
		};

		auto CalMInHxyHist = [&](const std::vector<uint32_t>& hist, uint32_t sampN,
			const std::vector<uint32_t>& nhist, uint32_t nsampN)
		{
			float MI = 0.0f;
			float Hxy = 0.0f;
			for (int i = 0; i < m_NumIntervals; ++i)
			{
				float px = static_cast<float>(hist[i]) / static_cast<float>(sampN);
				float py = static_cast<float>(nhist[i]) / static_cast<float>(nsampN);
				float pxy = static_cast<float>(hist[i] + nhist[i]) / static_cast<float>(sampN + nsampN);

				if (px == 0.0f || py == 0.0f)	continue;

				MI += pxy * log(pxy / (px * py));
				Hxy += -pxy * log(pxy);
			}
			MI = std::max(0.0f, MI);
			Hxy = std::max(0.0f, Hxy);

			return std::make_tuple(MI, Hxy);
		};

		// Prepare data members
		auto PrepareMember_SGMMUniHist = [&]()
		{
			for (int brickIdx = 0; brickIdx < numBricks; ++brickIdx)
			{
				auto brickCoord = GetCoordFromSeqIdx(brickIdx, m_BrickRes);

				glm::ivec3 brickBlockRes = glm::ivec3(
					m_XBlockGap[brickCoord.x + 1] - m_XBlockGap[brickCoord.x],
					m_YBlockGap[brickCoord.y + 1] - m_YBlockGap[brickCoord.y],
					m_ZBlockGap[brickCoord.z + 1] - m_ZBlockGap[brickCoord.z]);

				uint32_t numBrickBlocks = brickBlockRes.x * brickBlockRes.y * brickBlockRes.z;

				auto& brickBlockWeightsList = oldBlockBinWeightsPerBrick[brickIdx];
				brickBlockWeightsList.resize(numBrickBlocks, std::vector<float>(m_NumIntervals, 0.0f));

				auto& blockNeighborInfoList = blockNeighborInfoPerBrick[brickIdx];
				blockNeighborInfoList.resize(numBrickBlocks, mutualStruct());

				auto& gmmBrickCoeffs = m_GMMFile->getBrick(brickIdx).second.GMMListPerBlock;

				// initialize weights
				glm::ivec3 brickVolumeOrigin = glm::ivec3(
					m_XGap[brickCoord.x], 
					m_YGap[brickCoord.y], 
					m_ZGap[brickCoord.z]);
				for (uint32_t bblockIdx = 0; bblockIdx < numBrickBlocks; ++bblockIdx)
				{
					auto& weights = brickBlockWeightsList[bblockIdx];
					auto& gmmBrickBlockCoeffs = gmmBrickCoeffs[bblockIdx];
					std::transform(gmmBrickBlockCoeffs.begin(), gmmBrickBlockCoeffs.end(), weights.begin(),
						[&](const GMMBin& bin) { return bin.GetProb(); });

					auto& blockNeighborInfo = blockNeighborInfoList[bblockIdx];
					auto brickBlockCoord = GetCoordFromSeqIdx(bblockIdx, brickBlockRes);
					glm::ivec3 blockVolumeOrigin = brickVolumeOrigin + brickBlockCoord * glm::ivec3(m_BlockSize);

					glm::ivec3 blockSize = glm::ivec3(m_BlockSize);
					if (brickBlockCoord.x == brickBlockRes.x - 1)
						blockSize.x += (m_XGap[brickCoord.x + 1] - m_XGap[brickCoord.x]) % m_BlockSize;
					if (brickBlockCoord.y == brickBlockRes.y - 1)
						blockSize.y += (m_YGap[brickCoord.y + 1] - m_YGap[brickCoord.y]) % m_BlockSize;
					if (brickBlockCoord.z == brickBlockRes.z - 1)
						blockSize.z += (m_ZGap[brickCoord.z + 1] - m_ZGap[brickCoord.z]) % m_BlockSize;

					for (int x = 0; x < blockSize.x; ++x)
						for (int y = 0; y < blockSize.y; ++y)
							for (int z = 0; z < blockSize.z; ++z)
							{
								glm::ivec3 sampP = blockVolumeOrigin + glm::ivec3(x, y, z);
								uint32_t sampPSeqIdx = sampP.z * m_DataRes.y * m_DataRes.x + sampP.y * m_DataRes.x + sampP.x;

								float v = m_OriginDataBuffer[sampPSeqIdx];
								float dv = 1.0f;
								int I = std::floor(v / dv);
							}
				}
			}
		};
		PrepareMember_SGMMUniHist();

		std::vector<std::thread> tPool;

		auto Calculate_MIandPxy = [&](uint32_t brickIdx)
		{
			TINYVR_INFO("MIs & Pxy : Brick {0:>4}", brickIdx);
			auto brickCoord = GetCoordFromSeqIdx(brickIdx, m_BrickRes);

			glm::ivec3 brickBlockOrigin = glm::ivec3(
				m_XBlockGap[brickCoord.x],
				m_YBlockGap[brickCoord.y],
				m_ZBlockGap[brickCoord.z]);
			glm::ivec3 brickBlockRes = glm::ivec3(
				m_XBlockGap[brickCoord.x + 1] - m_XBlockGap[brickCoord.x],
				m_YBlockGap[brickCoord.y + 1] - m_YBlockGap[brickCoord.y],
				m_ZBlockGap[brickCoord.z + 1] - m_ZBlockGap[brickCoord.z]);

			uint32_t numBrickBlocks = brickBlockRes.x * brickBlockRes.y * brickBlockRes.z;
			auto& brickBlockWeightsList = oldBlockBinWeightsPerBrick[brickIdx];

			// calculate : mutual information and joint probability
			auto& blockNeighborInfoList = blockNeighborInfoPerBrick[brickIdx];
			for (uint32_t bblockIdx = 0; bblockIdx < numBrickBlocks; ++bblockIdx)
			{
				auto& blockNeighborInfo = blockNeighborInfoList[bblockIdx];

				glm::ivec3 blockSize = glm::ivec3(m_BlockSize);
				auto brickBlockCoord = GetCoordFromSeqIdx(bblockIdx, brickBlockRes);
				if (brickBlockCoord.x == brickBlockRes.x - 1)
					blockSize.x += (m_XGap[brickCoord.x + 1] - m_XGap[brickCoord.x]) % m_BlockSize;
				if (brickBlockCoord.y == brickBlockRes.y - 1)
					blockSize.y += (m_YGap[brickCoord.y + 1] - m_YGap[brickCoord.y]) % m_BlockSize;
				if (brickBlockCoord.z == brickBlockRes.z - 1)
					blockSize.z += (m_ZGap[brickCoord.z + 1] - m_ZGap[brickCoord.z]) % m_BlockSize;

				auto& weights = brickBlockWeightsList[bblockIdx];
				for (uint32_t d = 0; d < numDirs; ++d)
				{
					glm::ivec3 Dir = Dirs[d];
					glm::ivec3 nBlockGlobalCoord = brickBlockCoord + brickBlockOrigin + Dir;

					blockNeighborInfo.ExceedVol[d] = CheckExceedRange(nBlockGlobalCoord, blockRes);
					if (blockNeighborInfo.ExceedVol[d])		continue;

					glm::ivec3 nBrickIdx = SearchWhichBrickInIdx(nBlockGlobalCoord);
					glm::ivec3 nBrickOrigin = glm::ivec3(
						m_XBlockGap[nBrickIdx.x],
						m_YBlockGap[nBrickIdx.y],
						m_ZBlockGap[nBrickIdx.z]);
					glm::ivec3 nBrickRes = glm::ivec3(
						m_XBlockGap[nBrickIdx.x + 1] - m_XBlockGap[nBrickIdx.x],
						m_YBlockGap[nBrickIdx.y + 1] - m_YBlockGap[nBrickIdx.y],
						m_ZBlockGap[nBrickIdx.z + 1] - m_ZBlockGap[nBrickIdx.z]);

					uint32_t nBrickSeqIdx = GetSeqIdxFromCoord(nBrickIdx, m_BrickRes);
					auto& nBrickBlockWeightsList = oldBlockBinWeightsPerBrick[nBrickSeqIdx];

					glm::ivec3 nBlockOffsetCoord = nBlockGlobalCoord - nBrickOrigin;
					uint32_t nBlockIdx = nBlockOffsetCoord.z * nBrickRes.y * nBrickRes.x
						+ nBlockOffsetCoord.y * nBrickRes.x + nBlockOffsetCoord.x;
					auto& nWeights = nBrickBlockWeightsList[nBlockIdx];

					glm::ivec3 nBlockSize = glm::ivec3(m_BlockSize);
					auto nBrickBlockCoord = GetCoordFromSeqIdx(bblockIdx, brickBlockRes);
					if (nBlockOffsetCoord.x == nBrickRes.x - 1)
						nBlockSize.x += (m_XGap[nBrickIdx.x + 1] - m_XGap[nBrickIdx.x]) % m_BlockSize;
					if (nBlockOffsetCoord.y == nBrickRes.y - 1)
						nBlockSize.y += (m_YGap[nBrickIdx.y + 1] - m_YGap[nBrickIdx.y]) % m_BlockSize;
					if (nBlockOffsetCoord.z == nBrickRes.z - 1)
						nBlockSize.z += (m_ZGap[nBrickIdx.z + 1] - m_ZGap[nBrickIdx.z]) % m_BlockSize;

					auto [MI, Hxy] = CalMInHxy(weights, blockSize.x * blockSize.y * blockSize.z,
						nWeights, nBlockSize.x * nBlockSize.y * nBlockSize.z);
					blockNeighborInfo.MIs[d] = MI;
					blockNeighborInfo.Hxys[d] = Hxy;
				}
			}
		};

		for (uint32_t brickIdx = 0; brickIdx < numBricks; ++brickIdx)
			tPool.emplace_back(Calculate_MIandPxy, brickIdx);
		for (auto& t : tPool)	t.join();
		tPool.clear();
		//Calculate_MIandPxy(0);

		// EM Utility Functions
		using sampStruct = glm::ivec4;

		auto EM_Q = [&](const std::vector<glm::ivec4>& sampData,
			const std::vector<double>& gamma_ik, GMMBin& bin, int l, int r)->double
		{
			double Q = 0.0;

			uint32_t N = r - l + 1;
			uint32_t K = bin.GetKernelNum();

			double a_1 = 1.5 * std::log(2 * PI_D);

			for (uint32_t k = 0; k < K; ++k)
			{
				double ln_ak = std::log(bin.GetKernel(k).weight);
				glm::dmat3 sigk(0);
				sigk[0][0] = bin.GetKernel(k).funcs.x.var;
				sigk[1][1] = bin.GetKernel(k).funcs.y.var;
				sigk[2][2] = bin.GetKernel(k).funcs.z.var;
				double ln_sigk = std::log(std::abs(glm::determinant(sigk))) / 2.0;

				for (size_t n = 0; n < N; ++n)
				{
					double g_ik = gamma_ik[n * K + k];
					double ln_gik = std::log(g_ik);

					glm::dvec3 localP;
					localP.x = sampData[n + l].x;
					localP.y = sampData[n + l].y;
					localP.z = sampData[n + l].z;

					glm::dvec3 mu;
					mu.x = bin.GetKernel(k).funcs.x.mean;
					mu.y = bin.GetKernel(k).funcs.y.mean;
					mu.z = bin.GetKernel(k).funcs.z.mean;
					double sig = glm::dot(localP - mu, glm::inverse(sigk) * (localP - mu)) / 2.0;

					Q += g_ik * (ln_ak - ln_gik - a_1 - ln_sigk - sig);
				}
			}
			return Q;
		};

		auto EM_UpdateParam = [&](const std::vector<glm::ivec4>& sampData,
			int l, int r, const std::vector<double>& gamma_ik, GMMBin& bin, float eps)
		{
			uint32_t N = r - l + 1;
			uint32_t K = bin.GetKernelNum();

			for (uint32_t k = 0; k < K; ++k)
			{
				double sum_gk = 0.0;
				for (uint32_t n = 0; n < N; ++n)	sum_gk += gamma_ik[n * K + k];

				double a_k = sum_gk / N;

				// Update Mu
				glm::dvec3 mu(0.0);
				for (uint32_t n = 0; n < N; ++n)
				{
					glm::dvec3 localP(sampData[n + l].x, sampData[n + l].y, sampData[n + l].z);
					mu += gamma_ik[n * K + k] * localP;
				}

				mu /= sum_gk;

				// Update Sigma
				glm::dvec3 sigma(0.0);
				for (uint32_t n = 0; n < N; ++n)
				{
					glm::dvec3 localP(sampData[n + l].x, sampData[n + l].y, sampData[n + l].z);
					auto shift = localP - mu;

					sigma += gamma_ik[n * K + k] * (shift * shift);
				}
				sigma /= sum_gk;

				for (int i = 0; i < 3; ++i)
					sigma[i] = std::max(sigma[i], static_cast<double>(std::numeric_limits<float>::epsilon()));

				bin.SetKernelWeight(a_k, k);
				bin.SetKernelMeans(mu, k);
				bin.SetKernelVars(sigma, k);
			}
		};

		auto EM_Core = [&](const std::vector<glm::ivec4>& sampData, glm::ivec3 blockSize,
			GMMBin& bin, int l, int r, uint32_t I, float eps)->float
		{
			// Init parameters
			uint32_t N = r - l + 1;
			uint32_t K = bin.GetKernelNum();

			std::vector<double> gamma_ik(N * K, 0.0);
			double Q = 0.0;

			uint32_t iter = 0;
			while (iter++ < I)
			{
				// 1. E-step, estimate gamma_ik
				for (uint32_t n = 0; n < N; ++n)
				{
					glm::ivec3 localP(sampData[n + l].x, sampData[n + l].y, sampData[n + l].z);

					for (uint32_t k = 0; k < K; ++k)	gamma_ik[n * K + k] = bin.GetKernel(k)(localP);

					double g_sum = std::accumulate(gamma_ik.begin() + n * K, gamma_ik.begin() + (n + 1) * K, 0.0);

					constexpr double dEPS = std::numeric_limits<double>::epsilon();
					if (std::abs(g_sum) < dEPS)	for (uint32_t k = 0; k < K; ++k)	gamma_ik[n * K + k] = 1.0 / K;
					else						for (uint32_t k = 0; k < K; ++k)	gamma_ik[n * K + k] /= g_sum;
				}
				// 2. M-step, estimate Parameters
				EM_UpdateParam(sampData, l, r, gamma_ik, bin, eps);

				double Qt = EM_Q(sampData, gamma_ik, bin, l, r);
				if (std::abs(Qt - Q) < eps) { Q = Qt;	break;}
				else						  Q = Qt;
			}
			return Q;
		};

		unsigned long long uniSeed = std::chrono::steady_clock().now().time_since_epoch().count();
		auto EMRandEngineX = std::mt19937_64(uniSeed + 1);
		auto EMRandEngineY = std::mt19937_64(uniSeed + 2);
		auto EMRandEngineZ = std::mt19937_64(uniSeed + 3);
		std::uniform_real_distribution<double> floatUniformDistribX(-1.0f, 1.0f);
		std::uniform_real_distribution<double> floatUniformDistribY(-1.0f, 1.0f);
		std::uniform_real_distribution<double> floatUniformDistribZ(-1.0f, 1.0f);

		auto EM_PerBinDynamic = [&](const std::vector<glm::ivec4>& sampData, glm::ivec3 blockSize,
			GMMBin& bin, glm::ivec2 range, uint32_t K, uint32_t I, float eps)
		{
			std::vector<GMMBin> gmmsToBeChosen(K - 1);
			std::vector<double> Qs(K - 1, 0.0);
			std::vector<double> BICs(K - 1, 0.0);
			for (int k = 1; k < K; ++k)
			{
				for (int l = 0; l < k; ++l)
				{
					double x0 = floatUniformDistribX(EMRandEngineX) * static_cast<double>(blockSize.x) * 1.5;
					double y0 = floatUniformDistribY(EMRandEngineY) * static_cast<double>(blockSize.y) * 1.5;
					double z0 = floatUniformDistribZ(EMRandEngineZ) * static_cast<double>(blockSize.z) * 1.5;
					gmmsToBeChosen[k - 1].Push(GaussianKernel(1.0f / static_cast<float>(k),					  
						glm::dvec3(x0, y0, z0), glm::dvec3(8.0)));											  
				}
			}
			// EM for GMM of K-components
			for (int k = 1; k < K; ++k)
			{
				uint32_t numParams = k * (1 + 3 + 3 * (3 + 1) / 2);
				Qs[k - 1] = EM_Core(sampData, blockSize, gmmsToBeChosen[k - 1], range.x, range.y, I, eps);
				BICs[k - 1] = numParams * std::log(range.y - range.x + 1) - 2 * Qs[k - 1];
			}

			auto minBICIter = std::min_element(BICs.begin(), BICs.end());
			auto minBICIdx = minBICIter - BICs.begin();

			bin = gmmsToBeChosen[minBICIdx];
		};

		// Bin Weight Correction : Sampling
		auto Correct_NeighborSampling = [&](uint32_t brickIdx, float e_H, float e_MI)
		{
			TINYVR_INFO("Neighbor Sampling : Brick {0:>4}", brickIdx);
			auto brickCoord = GetCoordFromSeqIdx(brickIdx, m_BrickRes);

			glm::ivec3 brickBlockOrigin = glm::ivec3(
				m_XBlockGap[brickCoord.x],
				m_YBlockGap[brickCoord.y],
				m_ZBlockGap[brickCoord.z]);
			glm::ivec3 brickBlockRes = glm::ivec3(
				m_XBlockGap[brickCoord.x + 1] - m_XBlockGap[brickCoord.x],
				m_YBlockGap[brickCoord.y + 1] - m_YBlockGap[brickCoord.y],
				m_ZBlockGap[brickCoord.z + 1] - m_ZBlockGap[brickCoord.z]);

			uint32_t numBrickBlocks = brickBlockRes.x * brickBlockRes.y * brickBlockRes.z;

			auto& brickBlockWeightsList = oldBlockBinWeightsPerBrick[brickIdx];
			auto& blockNeighborInfoList = blockNeighborInfoPerBrick[brickIdx];

			for (uint32_t bblockIdx = 0; bblockIdx < numBrickBlocks; ++bblockIdx)
			{
				TINYVR_INFO("Neighbor Sampling : Brick {0:>4} -- Block {1:>4}", brickIdx, bblockIdx);

				auto& blockNeighborInfo = blockNeighborInfoList[bblockIdx];
				auto& weights = brickBlockWeightsList[bblockIdx];

				// calculate bin weight entropy H and check if should continue
				float bBinEntropy = 0.0f;
				for (int i = 0; i < m_NumIntervals; ++i)
				{
					float w = weights[i];
					if (w <= 0.0f)	continue;
					bBinEntropy += -w * log(w);
				}
				bBinEntropy = std::max(0.0f, bBinEntropy);
				blockNeighborInfo.binWeightEntropy = bBinEntropy;
				if (bBinEntropy < e_H)	continue;

				auto brickBlockCoord = GetCoordFromSeqIdx(bblockIdx, brickBlockRes);
				glm::ivec3 blockSize = m_GMMFile->getBlockSize(brickIdx, bblockIdx);

				std::vector<sampStruct> sampList;
				sampList.reserve(m_BlockSize * m_BlockSize * m_BlockSize * numDirs / 4);

				glm::ivec3 blockVolumeCoord = glm::ivec3(m_XGap[brickCoord.x], m_YGap[brickCoord.y], m_ZGap[brickCoord.z])
					+ brickBlockCoord * glm::ivec3(m_BlockSize);

				for (int i = 0; i < blockSize.x; ++i)
					for (int j = 0; j < blockSize.y; ++j)
						for (int k = 0; k < blockSize.z; ++k)
						{
							glm::ivec3 sampP = blockVolumeCoord + glm::ivec3(i, j, k);
							uint32_t sampPSeqIdx = GetSeqIdxFromCoord(sampP, m_DataRes);

							float v = m_OriginDataBuffer[sampPSeqIdx];
							float dv = 1.0f;
							int I = std::floor(v / dv);

							sampList.emplace_back(i, j, k, I);
						}

				std::unordered_set<uint32_t> neighborSampSeqIdxSet;

				for (uint32_t d = 0; d < numDirs; ++d)
				{
					if (blockNeighborInfo.ExceedVol[d])		continue;
					if (blockNeighborInfo.MIs[d] < e_MI)	continue;

					glm::ivec3 Dir = Dirs[d];
					glm::ivec3 nBlockGlobalCoord = brickBlockCoord + brickBlockOrigin + Dir;

					glm::ivec3 nBrickIdx = SearchWhichBrickInIdx(nBlockGlobalCoord);
					glm::ivec3 nBrickOrigin = glm::ivec3(
						m_XBlockGap[nBrickIdx.x],
						m_YBlockGap[nBrickIdx.y],
						m_ZBlockGap[nBrickIdx.z]);
					glm::ivec3 nBrickRes = glm::ivec3(
						m_XBlockGap[nBrickIdx.x + 1] - m_XBlockGap[nBrickIdx.x],
						m_YBlockGap[nBrickIdx.y + 1] - m_YBlockGap[nBrickIdx.y],
						m_ZBlockGap[nBrickIdx.z + 1] - m_ZBlockGap[nBrickIdx.z]);

					uint32_t nBrickSeqIdx = GetSeqIdxFromCoord(nBrickIdx, m_BrickRes);
					auto& nBrickBlockWeightsList = oldBlockBinWeightsPerBrick[nBrickSeqIdx];

					glm::ivec3 nBlockOffsetCoord = nBlockGlobalCoord - nBrickOrigin;
					uint32_t nBlockIdx = GetSeqIdxFromCoord(nBlockOffsetCoord, nBrickRes);
					auto& nWeights = nBrickBlockWeightsList[nBlockIdx];

					auto nBrickBlockCoord = GetCoordFromSeqIdx(bblockIdx, brickBlockRes);
					glm::ivec3 nBlockSize = m_GMMFile->getBlockSize(nBrickSeqIdx, nBlockIdx);

					float maxHxy = *std::max_element(blockNeighborInfo.Hxys.begin(), blockNeighborInfo.Hxys.end());
					float sampRate = (maxHxy == 0.0f) ? 0.0f : 1.0f - blockNeighborInfo.Hxys[d] / maxHxy;
					int sampN = static_cast<int>(std::floor(sampRate * nBlockSize.x * nBlockSize.y * nBlockSize.z));

					glm::ivec3 nBlockVolumeCoord = nBlockOffsetCoord * glm::ivec3(m_BlockSize) +
						glm::ivec3(m_XGap[nBrickIdx.x], m_YGap[nBrickIdx.y], m_ZGap[nBrickIdx.z]);

					// sample with replacement
					unsigned long long int seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
					std::mt19937_64 rdEngineX(seed + 1);
					std::mt19937_64 rdEngineY(seed + 2);
					std::mt19937_64 rdEngineZ(seed + 3);
					std::uniform_int_distribution<int> rdX(0, nBlockSize.x - 1);
					std::uniform_int_distribution<int> rdY(0, nBlockSize.y - 1);
					std::uniform_int_distribution<int> rdZ(0, nBlockSize.z - 1);

					for (int i = 0; i < sampN; ++i)
					{
						int x = rdX(rdEngineX);
						int y = rdY(rdEngineY);
						int z = rdZ(rdEngineZ);

						glm::ivec3 nsampP = nBlockVolumeCoord + glm::ivec3(x, y, z);
						uint32_t nsampPSeqIdx = GetSeqIdxFromCoord(nsampP, m_DataRes);
						neighborSampSeqIdxSet.insert(nsampPSeqIdx);

						float v = m_OriginDataBuffer[nsampPSeqIdx];
						float dv = 1.0f;
						int I = std::floor(v / dv);

						sampList.emplace_back(nsampP.x - blockVolumeCoord.x,
											nsampP.y - blockVolumeCoord.y,
											nsampP.z - blockVolumeCoord.z, I);
					}
					
					// sample without replacement
					//unsigned long long int seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
					//std::mt19937_64 shuffleEngine(seed);
					//std::uniform_int_distribution<int> rdX;

					//std::vector<sampStruct> nSampWithoutRep;
					//nSampWithoutRep.reserve(nBlockSize.x * nBlockSize.y * nBlockSize.z);
					//for (int x = 0; x < nBlockSize.x; ++x)
					//	for (int y = 0; y < nBlockSize.y; ++y)
					//		for (int z = 0; z < nBlockSize.z; ++z)
					//		{
					//			glm::ivec3 nsampP = nBlockVolumeCoord + glm::ivec3(x, y, z);
					//			uint32_t sampPSeqIdx = GetSeqIdxFromCoord(sampP, m_DataRes);

					//			float v = m_OriginDataBuffer[nsampPSeqIdx];
					//			float dv = 1.0f;
					//			int I = std::floor(v / dv);

					//			nSampWithoutRep.emplace_back(nsampP.x, nsampP.y, nsampP.z, I);
					//		}

					//std::shuffle(nSampWithoutRep.begin(), nSampWithoutRep.end(), shuffleEngine);
					//for (int i = 0; i < sampN; ++i)	sampList.emplace_back(
					//	nSampWithoutRep[i].x - blockVolumeCoord.x,
					//	nSampWithoutRep[i].y - blockVolumeCoord.y,
					//	nSampWithoutRep[i].z - blockVolumeCoord.z, nSampWithoutRep[i].w);
				}

				if (sampList.size() == blockSize.x * blockSize.y * blockSize.z)	continue;

				std::sort(sampList.begin(), sampList.end(),
					[&](const glm::ivec4& lhs, const glm::ivec4& rhs) {	return lhs.w < rhs.w; });

				std::vector<GMMBin> gmmBinList(m_NumIntervals);
				for (size_t binIdx = 0; binIdx < m_NumIntervals; ++binIdx)
				{
					auto range = EM_Util::ValueRange(sampList, binIdx);
					if (range == glm::ivec2(-1, -1))	continue;

					uint32_t Nk = range.y - range.x + 1;
					uint32_t N = sampList.size();
					auto& bin = gmmBinList[binIdx];

					double binWeight = static_cast<double>(Nk) / N;
					EM_PerBinDynamic(sampList, blockSize, bin, range, 4, 1000, 1e-4);

					double binWeightSum = 0.0;
					for (int x = 0; x < blockSize.x; ++x)
						for (int y = 0; y < blockSize.y; ++y)
							for (int z = 0; z < blockSize.z; ++z)
							{
								glm::dvec3 sampP = glm::dvec3(x, y, z);
								binWeightSum += bin.EvalWithoutProb(sampP);
							}

					for (auto pointSeqIdx : neighborSampSeqIdxSet)
					{
						glm::ivec3 pointVolumeCoord = GetCoordFromSeqIdx(pointSeqIdx, m_DataRes);
						glm::dvec3 nsampP = pointVolumeCoord - blockVolumeCoord;
						binWeightSum += bin.EvalWithoutProb(nsampP);
					}

					gmmBinList[binIdx].SetProb(binWeight / binWeightSum);
				}
				m_GMMFile->UpdateGMMBinCoeffs(brickIdx, bblockIdx, gmmBinList);
			}
		};

		float e_H = 0.0f;
		float e_MI = 0.0f;
		for (uint32_t brickIdx = 0; brickIdx < numBricks; ++brickIdx)
			tPool.emplace_back(Correct_NeighborSampling, brickIdx, e_H, e_MI);
		for (auto& t : tPool)	t.join();
		tPool.clear();

		return;
	}

	void vrGMMLayer::initRCComponent()
	{
		TINYVR_PROFILE_FUNCTION();

		m_TrackBall.reset(new tinyvr::vrTrackBall_CHEN(m_MousePos));
		m_CameraController.reset(new tinyvr::vrPerspectiveCameraController(
			{ 0.0f, 0.0f, 2.0f }, m_ScrSize.x, m_ScrSize.y));
		{	// transfer function
			m_TFVolume = tinyvr::vrTransferFunction::Create(m_TFVolumeTex);
			m_TFVolumeWidget = tinyvr::vrTransferFunctionWidget::Create(m_TFVolume, "Volume");

			m_TFEntropy = tinyvr::vrTransferFunction::Create(m_TFEntroyTex);
			m_TFEntropyWidget = tinyvr::vrTransferFunctionWidget::Create(m_TFEntropy, "Entropy");
		}

		{	// Shader here~
			m_ProjShader = tinyvr::vrShader::Create("resources/shaders/render/RC/rayProj_vert.glsl", "resources/shaders/render/RC/rayProj_frag.glsl");
			m_RCShader = tinyvr::vrShader::Create("resources/shaders/render/RC/rayCasting_vert.glsl", "resources/shaders/render/RC/rayCasting_frag.glsl");
			m_AMRRCShader = tinyvr::vrShader::Create("resources/shaders/render/RC/RC_AMR_vert.glsl", "resources/shaders/render/RC/RC_AMR_frag.glsl");
			m_DisplayShader = tinyvr::vrShader::Create("resources/shaders/render/RC/renderTexture_vert.glsl", "resources/shaders/render/RC/renderTexture_frag.glsl");
		}

		{	// vertex array of cube
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
				{ tinyvr::ShaderDataType::Float3, "a_TexCoord" }});
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
			float texVerts[] =
			{
				-1.0f,  1.0f, 0.0f, 1.0f,
				-1.0f, -1.0f, 0.0f, 0.0f,
				 1.0f, -1.0f, 1.0f, 0.0f,
				 1.0f,  1.0f, 1.0f, 1.0f
			};

			// Cube Vertex Buffer
			tinyvr::vrRef<tinyvr::vrVertexBuffer> texVB;
			texVB = tinyvr::vrVertexBuffer::Create(texVerts, sizeof(texVerts));

			m_DisplayVA = tinyvr::vrVertexArray::Create();
			texVB->SetLayout({
				{ tinyvr::ShaderDataType::Float2, "a_Position" },
				{ tinyvr::ShaderDataType::Float2, "a_TexCoord" } });
			m_DisplayVA->AddVertexBuffer(texVB);

			// Cube Indices Buffer
			uint32_t texIndices[] = {
				0, 1, 2,
				2, 3, 0 };

			tinyvr::vrRef<tinyvr::vrIndexBuffer> texIB;
			texIB = tinyvr::vrIndexBuffer::Create(texIndices, sizeof(texIndices) / sizeof(uint32_t));
			m_DisplayVA->SetIndexBuffer(texIB);
		}

		{
			auto maxSPAN = std::max({ m_DataRes.x, m_DataRes.y, m_DataRes.z });
			glm::vec3 scale = glm::vec3(m_DataRes) / glm::vec3(maxSPAN);
			m_ModelMat = glm::scale(glm::mat4(1.0f), scale);
			m_ModelMat = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), { 0.0f, 1.0f, 0.0f }) * m_ModelMat;

			// Isabel : none
			// Asteroid : v02
			//m_ModelMat = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), { 0.0f, 1.0f, 0.0f }) * m_ModelMat;
			// Asteroid : v03
			//m_ModelMat = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), { 0.0f, 1.0f, 0.0f }) * m_ModelMat;
			// lap3d
			m_ModelMat = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), { 0.0f, 1.0f, 0.0f }) * m_ModelMat;
		}

		{
			tinyvr::vrFrameBufferSpecification fbspec;
			fbspec.Width = 1280;
			fbspec.Height = 720;

			m_ProjInFB = tinyvr::vrFrameBuffer::Create(fbspec);
			m_ProjInFB->CheckStatus();
			
			m_ProjOutFB = tinyvr::vrFrameBuffer::Create(fbspec);
			m_ProjOutFB->CheckStatus();

			m_RenderFB = tinyvr::vrFrameBuffer::Create(fbspec);
			m_RenderFB->CheckStatus();

			m_DisplayFB = tinyvr::vrFrameBuffer::Create(fbspec);
			m_DisplayFB->CheckStatus();
		}
	}
	
	void vrGMMLayer::initEntropyComponent()
	{
		{
			m_LocalEntropyTex = tinyvr::vrTexture3D::Create(m_DataRes.x, m_DataRes.y, m_DataRes.z,
				tinyvr::vrTextureFormat::TEXTURE_FMT_RED);
			m_LocalEntropyTex->SetData(m_DataRes.x * m_DataRes.y * m_DataRes.z, nullptr);
			
			{	// calculate entropy using local histogram
				m_EntropyLocalHistCompShader = tinyvr::vrShader::CreateComp("resources/shaders/compute/calculateLocalHistEntropy_comp.glsl");
				m_EntropyLocalHistCompShader->Bind();

				m_LocalEntropyTex->BindImage(0, tinyvr::vrTexImageAccess::TEXTURE_IMAGE_ACCESS_WRITEONLY);
				m_VolumeTex->BindUnit(1);

				m_EntropyLocalHistCompShader->SetFloat("vMin", m_ValueRange.x);
				m_EntropyLocalHistCompShader->SetFloat("vMax", m_ValueRange.y);
				m_EntropyLocalHistCompShader->SetInt("NumIntervals", m_NumIntervals);
				m_EntropyLocalHistCompShader->SetInt("StencilSize", 5);

				std::dynamic_pointer_cast<tinyvr::vrOpenGLCompShader>(m_EntropyLocalHistCompShader)->Compute(m_DataRes);
			}

			//{	// calculate GMM differential entropy
			//	tinyvr::vrRef<tinyvr::vrShader> SGMMEntropyCompShader =
			//		tinyvr::vrShader::CreateComp("resources/shaders/compute/calculateSGMMEntropy_comp.glsl");
			//	SGMMEntropyCompShader->Bind();

			//	m_LocalEntropyTex->BindImage(0, tinyvr::vrTexImageAccess::TEXTURE_IMAGE_ACCESS_WRITEONLY);
			//	for (int i = 0; i < m_GMMCoeffTexturesList.size(); ++i)		m_GMMCoeffTexturesList[i]->BindUnit(i + 1);

			//	SGMMEntropyCompShader->SetInt("B", m_BlockSize);
			//	SGMMEntropyCompShader->SetFloat("vMin", m_ValueRange.x);
			//	SGMMEntropyCompShader->SetFloat("vMax", m_ValueRange.y);
			//	SGMMEntropyCompShader->SetInt("NumIntervals", m_NumIntervals);
			//	SGMMEntropyCompShader->SetInt("NumBricks", m_BrickRes.x * m_BrickRes.y * m_BrickRes.z);

			//	for (int i = 0; i < m_BrickRes.x; ++i)
			//		for (int j = 0; j < m_BrickRes.y; ++j)
			//			for (int k = 0; k < m_BrickRes.z; ++k)
			//			{
			//				auto O = glm::ivec3(m_XGap[i], m_YGap[j], m_ZGap[k]);
			//				auto R = glm::ivec3(m_XGap[i + 1] - m_XGap[i],
			//									m_YGap[j + 1] - m_YGap[j],
			//									m_ZGap[k + 1] - m_ZGap[k]);

			//				SGMMEntropyCompShader->SetInt3("O", O);
			//				SGMMEntropyCompShader->SetInt3("R", R);
			//				SGMMEntropyCompShader->SetInt("BRICK_IDX", k * m_BrickRes.y * m_BrickRes.x + j * m_BrickRes.x + i);

			//				std::dynamic_pointer_cast<tinyvr::vrOpenGLCompShader>(SGMMEntropyCompShader)->Compute(R);
			//			}
			//}
		}
		m_EntopyHistogram = m_LocalEntropyTex->GetHistogram(m_NumIntervals);
		auto mM = std::minmax_element(m_EntopyHistogram.begin(), m_EntopyHistogram.end());
		m_EntropyHistRange = { *(mM.first), *(mM.second) };

		m_EntropyRange = m_LocalEntropyTex->GlobalMinMaxVal();
		TINYVR_CORE_INFO("Entropy range : [{0:>8.4}, {1:>8.4}]", m_EntropyRange.x, m_EntropyRange.y);
	}
	
	void vrGMMLayer::constructOctree()
	{
		{
			uint32_t m = std::max({ m_DataRes.x, m_DataRes.y, m_DataRes.z });
			maxOctreeDepth = static_cast<uint32_t>(std::floor(log2(1.0 * m / patchSize))) + 1;
			treeMaxDepth = maxOctreeDepth + 1;

			if (treeMaxDepth <= 6)	treeTexHeight = treeMaxDepth;
			else
			{
				treeTexHeight = 6;
				for (int i = 0; i < treeMaxDepth - 6; ++i)
					treeTexHeight += static_cast<uint32_t>(std::pow(8, i + 1));
			}
		}

		{	// [Node Pool]	r: [m:1] [f:1] [off_x:15, off_y:15]
			// [Pos Pool]	r: x0  g : y0  b : z0  w : node size
			m_TreeNodeTex = tinyvr::vrTexture2D::Create(treeTexWidth, treeTexHeight,
				tinyvr::vrTextureFormat::TEXTURE_FMT_RED, tinyvr::vrTextureType::TEXTURE_TYPE_U32I);
			m_TreePosTex = tinyvr::vrTexture2D::Create(treeTexWidth, treeTexHeight,
				tinyvr::vrTextureFormat::TEXTURE_FMT_RGBA, tinyvr::vrTextureType::TEXTURE_TYPE_FLT32);

			m_TreeNodeTex->SetData(treeTexWidth * treeTexHeight);
			m_TreePosTex->SetData(treeTexWidth * treeTexHeight);
		}

		{
			m_ConOctreeFlagCompShader = tinyvr::vrShader::CreateComp("resources/shaders/compute/reconstructTBAMR_flag_comp.glsl");
			m_ConOctreeRefineCompShader = tinyvr::vrShader::CreateComp("resources/shaders/compute/reconstructTBAMR_refine_comp.glsl");

			auto eValveLevel = [&](std::vector<float>& eBuffer, float R, glm::vec2 range)
			{
				for (int i = 0; i < eBuffer.size(); ++i)
					eBuffer[i] = (range.y - range.x) * (1.0 - pow(R, i)) + range.x;

				fmt::print("Entropy Param : [{0:>8.6}]\n", fmt::join(eBuffer, ","));
			};

			std::vector<float> entropyValves(maxOctreeDepth, 0.0f);

			{	// Data 1 : Isabel
				//float evalveRatio = 0.685;
				//float evalveRatio = 0.0;
				//float evalveRatio = 0.72;
				//float evalveRatio = 0.78;
				//eValveLevel(entropyValves, evalveRatio, { 0.67, m_EntropyRange.y });
			}
			
			{	// Data 2 : Deep Water Impact (v02)
				//float evalveRatio = 0.75;
				//float evalveRatio = 0.72;
				//eValveLevel(entropyValves, evalveRatio, { 0.6, m_EntropyRange.y });
				//{
					//float evalveRatio = 0.80;
					//eValveLevel(entropyValves, evalveRatio, m_EntropyRange);
				//}
			}

			{	// Data 3 : Deep Water Impact (snd)
				//float evalveRatio = 0.82;
				//float evalveRatio = 0.9;
				//eValveLevel(entropyValves, evalveRatio, { -0.5, m_EntropyRange.y });
				//{
					//float evalveRatio = 0.9;
					//eValveLevel(entropyValves, evalveRatio, m_EntropyRange);
				//}
			}

			{	// Data 4 : Lap 3D
				float evalveRatio = 0.8;
				eValveLevel(entropyValves, evalveRatio, { 0.0f, m_EntropyRange.y });
			}

			{	// Data 5 : shock
				//float evalveRatio = 0.8;
				//eValveLevel(entropyValves, evalveRatio, { -0.5, m_EntropyRange.y });
			}

			{	// Data 6 : plane
				//float evalveRatio = 0.8;
				//eValveLevel(entropyValves, evalveRatio, { -0.5, m_EntropyRange.y });
			}

			m_AMRMinMaxEntropyRangeTex = tinyvr::vrTexture2D::Create(maxOctreeDepth, 2,
				tinyvr::vrTextureFormat::TEXTURE_FMT_RED, tinyvr::vrTextureType::TEXTURE_TYPE_32I);
			m_AMRMinMaxEntropyRangeTex->SetData(maxOctreeDepth * 2);

			std::vector<glm::vec2> m_entropyRange(maxOctreeDepth);

    		for (int i = 0; i < treeMaxDepth - 1; ++i)
			{
				// step 1. check & flag
				m_ConOctreeFlagCompShader->Bind();
				
				m_TreeNodeTex->BindImage(0, tinyvr::vrTexImageAccess::TEXTURE_IMAGE_ACCESS_READWRITE);
				m_TreePosTex->BindImage(1, tinyvr::vrTexImageAccess::TEXTURE_IMAGE_ACCESS_READWRITE);
				m_AMRMinMaxEntropyRangeTex->BindImage(2, tinyvr::vrTexImageAccess::TEXTURE_IMAGE_ACCESS_READWRITE);
				m_LocalEntropyTex->BindUnit(3);

				m_ConOctreeFlagCompShader->SetInt("texW", treeTexWidth);
				m_ConOctreeFlagCompShader->SetInt("PS", patchSize);

				m_ConOctreeFlagCompShader->SetFloat("eValve", entropyValves[i]);
				m_ConOctreeFlagCompShader->SetInt("currentLevel", i);  

				std::dynamic_pointer_cast<tinyvr::vrOpenGLCompShader>(m_ConOctreeFlagCompShader)
					->Compute({ std::pow(8, i), 1, 1});

				// step 1.5. fetch min & max Value
				float minEntropy = tinyvr::vrFrameBuffer::ReadPixelUI(m_AMRMinMaxEntropyRangeTex, glm::ivec2{ i, 0 }) / 1.0e8;
				float maxEntropy = tinyvr::vrFrameBuffer::ReadPixelUI(m_AMRMinMaxEntropyRangeTex, glm::ivec2{ i, 1 }) / 1.0e8;
				m_entropyRange[i] = { minEntropy, maxEntropy };

				// step 2. refinement
				m_ConOctreeRefineCompShader->Bind();

				m_TreeNodeTex->BindImage(0, tinyvr::vrTexImageAccess::TEXTURE_IMAGE_ACCESS_READWRITE);
				m_TreePosTex->BindImage(1, tinyvr::vrTexImageAccess::TEXTURE_IMAGE_ACCESS_READWRITE);

				m_ConOctreeRefineCompShader->SetInt("texW", treeTexWidth);
				m_ConOctreeRefineCompShader->SetInt("PS", patchSize);
				m_ConOctreeRefineCompShader->SetInt("currentLevel", i);

				std::dynamic_pointer_cast<tinyvr::vrOpenGLCompShader>(m_ConOctreeRefineCompShader)
					->Compute({ std::pow(8, i), 8, 1 });
			}

			std::cout << "Entropy Range : ";
			for (auto p : m_entropyRange)
				std::cout << "[" << p.x << ", " << p.y << "] ";
			std::cout << "\n";

			{	// Octree Statistical
				m_CalAMRNodesCompShader = tinyvr::vrShader::CreateComp("resources/shaders/compute/calculateOctreeNodes_comp.glsl");
				m_CalAMRNodesCompShader->Bind();

				m_TreeNodeTex->BindImage(0, tinyvr::vrTexImageAccess::TEXTURE_IMAGE_ACCESS_READWRITE);
				m_CalAMRNodesCompShader->SetInt("texW", 32768);
				m_CalAMRNodesCompShader->SetInt("PS", patchSize);

				for (int i = 0; i < treeMaxDepth; ++i)
				{
					m_CalAMRNodesCompShader->SetInt("L", i);
					std::dynamic_pointer_cast<tinyvr::vrOpenGLCompShader>(m_CalAMRNodesCompShader)
						->Compute({ std::pow(8, i), 1, 1 });
				}

				auto nInnerNodes = tinyvr::vrFrameBuffer::ReadPixelUI(m_TreeNodeTex, glm::ivec2(1, 0));
				auto nLeafNodes = tinyvr::vrFrameBuffer::ReadPixelUI(m_TreeNodeTex, glm::ivec2(2, 0));

				TINYVR_INFO("Build TB-AMR Finished, with inner nodes = {0:>8}, "
					"leaf nodes = {1:>8}, total {2:>8} nodes, {3:>10} uints occupied\n",
					nInnerNodes, nLeafNodes, nInnerNodes + nLeafNodes, 
					nInnerNodes + (patchSize * patchSize * patchSize) * nLeafNodes / 4);
			}
		}
	}

	void vrGMMLayer::reconByAMR()
	{
		m_ReconVolumeByAMRTex = tinyvr::vrTexture3D::Create(m_DataRes.x, m_DataRes.y, m_DataRes.z,
			tinyvr::vrTextureFormat::TEXTURE_FMT_RED, tinyvr::vrTextureType::TEXTURE_TYPE_FLT32);
		m_ReconVolumeByAMRTex->SetData(m_DataRes.x * m_DataRes.y * m_DataRes.z);
		
		m_CalAMRReconCompShader = tinyvr::vrShader::CreateComp("resources/shaders/compute/reconstruct3DVolumeByAMR_Comp.glsl");
		m_CalAMRReconCompShader->Bind();

		m_ReconVolumeByAMRTex->BindImage(0, tinyvr::vrTexImageAccess::TEXTURE_IMAGE_ACCESS_WRITEONLY);
		m_VolumeTex->BindUnit(1);

		m_TreeNodeTex->BindUnit(2);
		m_TreePosTex->BindUnit(3);

		m_CalAMRReconCompShader->SetInt3("dataRes", m_DataRes);
		m_CalAMRReconCompShader->SetInt("B", m_BlockSize);
		m_CalAMRReconCompShader->SetInt("PS", patchSize);

		std::dynamic_pointer_cast<tinyvr::vrOpenGLCompShader>(m_CalAMRReconCompShader)->Compute(m_DataRes);

		//m_CalAMRReconCompShader = tinyvr::vrShader::CreateComp("resources/shaders/compute/reconstructTBAMR_SGMM_comp.glsl");
		//m_CalAMRReconCompShader->Bind();

		//m_ReconVolumeByAMRTex->BindImage(0, tinyvr::vrTexImageAccess::TEXTURE_IMAGE_ACCESS_WRITEONLY);

		//m_TreeNodeTex->BindUnit(1);
		//m_TreePosTex->BindUnit(2);

		//for (int i = 0; i < m_GMMCoeffTexturesList.size(); ++i)
		//	m_GMMCoeffTexturesList[i]->BindUnit(i + 3);

		//m_CalAMRReconCompShader->SetInt("B", m_BlockSize);
		//m_CalAMRReconCompShader->SetInt("PS", patchSize);

		//m_CalAMRReconCompShader->SetInt("NumIntervals", m_NumIntervals);
		//m_CalAMRReconCompShader->SetInt("NumBricks", m_BrickRes.x * m_BrickRes.y * m_BrickRes.z);

		//m_CalAMRReconCompShader->SetInt3("BrickRes", m_BrickRes);
		//m_CalAMRReconCompShader->SetInt3("DataRes", m_DataRes);

		//m_CalAMRReconCompShader->SetInts("XGap", m_XGap, m_BrickRes.x + 1);
		//m_CalAMRReconCompShader->SetInts("YGap", m_YGap, m_BrickRes.y + 1);
		//m_CalAMRReconCompShader->SetInts("ZGap", m_ZGap, m_BrickRes.z + 1);

		//m_CalAMRReconCompShader->SetInts("XBlockGap", m_XBlockGap, m_BrickRes.x + 1);
		//m_CalAMRReconCompShader->SetInts("YBlockGap", m_YBlockGap, m_BrickRes.y + 1);
		//m_CalAMRReconCompShader->SetInts("ZBlockGap", m_ZBlockGap, m_BrickRes.z + 1);

		//for (int i = 0; i < m_BrickRes.x; ++i)
		//	for (int j = 0; j < m_BrickRes.y; ++j)
		//		for (int k = 0; k < m_BrickRes.z; ++k)
		//		{
		//			auto O = glm::ivec3(m_XGap[i], m_YGap[j], m_ZGap[k]);
		//			auto R = glm::ivec3(m_XGap[i + 1], m_YGap[j + 1], m_ZGap[k + 1]) - O;

		//			m_CalAMRReconCompShader->SetInt3("O", O);
		//			m_CalAMRReconCompShader->SetInt3("R", R);

		//			m_CalAMRReconCompShader->SetInt("BRICK_IDX", k * m_BrickRes.y * m_BrickRes.x + j * m_BrickRes.x + i);

		//			std::dynamic_pointer_cast<tinyvr::vrOpenGLCompShader>(m_CalAMRReconCompShader)->Compute(R);
		//		}
	}

	void gmm::vrGMMLayer::measureRMSE()
	{
		uint32_t m = std::min({ m_DataRes.x, m_DataRes.y, m_DataRes.z });
		m_RMSENSSIMTex = tinyvr::vrTexture2D::Create(5, 2 + static_cast<uint32_t>(std::ceil(std::log2(m))),
			tinyvr::vrTextureFormat::TEXTURE_FMT_RED, tinyvr::vrTextureType::TEXTURE_TYPE_FLT32);
		m_RMSENSSIMTex->SetData(2);

		m_MeasureRMSECompShader = tinyvr::vrShader::CreateComp("resources/shaders/compute/statisticalMSE_comp.glsl");
		m_MeasureRMSECompShader->Bind();

		m_RMSENSSIMTex->BindImage(0, tinyvr::vrTexImageAccess::TEXTURE_IMAGE_ACCESS_READWRITE);

		//m_VolumeTex->BindUnit(1);
		m_OriginVolumeTex->BindUnit(1);
		m_ReconVolumeByAMRTex->BindUnit(2);

		std::dynamic_pointer_cast<tinyvr::vrOpenGLCompShader>(m_MeasureRMSECompShader)->Compute(m_DataRes);

		float sums = tinyvr::vrFrameBuffer::ReadPixelF(m_RMSENSSIMTex, glm::ivec2(0, 0));
		float MSE = sums / (m_DataRes.x * m_DataRes.y * m_DataRes.z);
		float RMSE = std::sqrt(MSE);

		TINYVR_INFO("Calculate RMSE Finished, MSE = {0:>10.6}, RMSE = {1:>10.6}", MSE, RMSE);
	}

	void gmm::vrGMMLayer::measureSSIM()
	{
		uint32_t m = std::min({ m_DataRes.x, m_DataRes.y, m_DataRes.z });
		uint32_t mipmapLVL = static_cast<uint32_t>(std::ceil(std::log2(m)));

		m_DownSamplingCompShader = tinyvr::vrShader::CreateComp("resources/shaders/compute/statisticalDownSampling_comp.glsl");

		m_MeasureSSIMMeanCompShader = tinyvr::vrShader::CreateComp("resources/shaders/compute/statisticalMSSIM_mean_comp.glsl");
		m_MeasureSSIMVar2CompShader = tinyvr::vrShader::CreateComp("resources/shaders/compute/statisticalMSSIM_var2_comp.glsl");
		
		int L = 255;
		double K1 = 0.01;
		double K2 = 0.03;
		double C1 = pow(K1 * L, 2.0);
		double C2 = pow(K2 * L, 2.0);
		double C3 = C2 / 2;

		std::vector<float> SSIM(mipmapLVL, 0.0f);
		std::vector<float> gamma(mipmapLVL, 0.0f);

		std::vector<glm::vec3> lcs(mipmapLVL, glm::vec3(0.0f));

		glm::ivec3 res = m_DataRes;
		for (uint32_t i = 0; i < mipmapLVL; ++i)
		{
			// 0. initialization
			glm::ivec2 mxIter = { 0, i + 1 };
			glm::ivec2 myIter = { 1, i + 1 };

			glm::ivec2 sxIter = { 2, i + 1 };
			glm::ivec2 syIter = { 3, i + 1 };
			glm::ivec2 sxyIter = { 4, i + 1 };

			uint32_t NSamples = res.x * res.y * res.z;

			// 1. mean
			m_MeasureSSIMMeanCompShader->Bind();

			m_RMSENSSIMTex->BindImage(0, tinyvr::vrTexImageAccess::TEXTURE_IMAGE_ACCESS_READWRITE);
			m_CopyVolumeTex->BindUnit(1);
			m_ReconVolumeByAMRTex->BindUnit(2);

			m_MeasureSSIMMeanCompShader->SetInt("mipmapLVL", i);
			m_MeasureSSIMMeanCompShader->SetInt3("R", res);

			std::dynamic_pointer_cast<tinyvr::vrOpenGLCompShader>(m_MeasureSSIMMeanCompShader)->Compute(res);

			double mx = tinyvr::vrFrameBuffer::ReadPixelF(m_RMSENSSIMTex, mxIter) / NSamples;
			double my = tinyvr::vrFrameBuffer::ReadPixelF(m_RMSENSSIMTex, myIter) / NSamples;

			// 2. vars
			m_MeasureSSIMVar2CompShader->Bind();

			m_RMSENSSIMTex->BindImage(0, tinyvr::vrTexImageAccess::TEXTURE_IMAGE_ACCESS_READWRITE);
			m_CopyVolumeTex->BindUnit(1);
			m_ReconVolumeByAMRTex->BindUnit(2);

			m_MeasureSSIMVar2CompShader->SetInt("mipmapLVL", i);
			m_MeasureSSIMVar2CompShader->SetInt3("R", res);

			m_MeasureSSIMVar2CompShader->SetFloat("mx", mx);
			m_MeasureSSIMVar2CompShader->SetFloat("my", my);

			std::dynamic_pointer_cast<tinyvr::vrOpenGLCompShader>(m_MeasureSSIMVar2CompShader)->Compute(res);

			double sx = std::sqrt(tinyvr::vrFrameBuffer::ReadPixelF(m_RMSENSSIMTex, sxIter) / (NSamples - 1));
			double sy = std::sqrt(tinyvr::vrFrameBuffer::ReadPixelF(m_RMSENSSIMTex, syIter) / (NSamples - 1));
			double sxy = tinyvr::vrFrameBuffer::ReadPixelF(m_RMSENSSIMTex, sxyIter) / (NSamples - 1);

			// 3. LCS
			lcs[i].x = (2.0 * mx * my + C1) / (mx * mx + my * my + C1);
			lcs[i].y = (2.0 * sx * sy + C2) / (sx * sx + sy * sy + C2);
			lcs[i].z = (sxy + C3) / (sx * sy + C3);

			SSIM[i] = lcs[i].x * lcs[i].y * lcs[i].z;

			// 4. Down-sampling
			auto UpRes = res;
			m_DownSamplingCompShader->Bind();

			m_CopyVolumeTex->BindImage(0, tinyvr::vrTexImageAccess::TEXTURE_IMAGE_ACCESS_READWRITE);
			m_DownSamplingCompShader->SetInt3("UpRes", UpRes);
			std::dynamic_pointer_cast<tinyvr::vrOpenGLCompShader>(m_DownSamplingCompShader)->Compute(res);

			m_ReconVolumeByAMRTex->BindImage(0, tinyvr::vrTexImageAccess::TEXTURE_IMAGE_ACCESS_READWRITE);
			m_DownSamplingCompShader->SetInt3("UpRes", UpRes);
			std::dynamic_pointer_cast<tinyvr::vrOpenGLCompShader>(m_DownSamplingCompShader)->Compute(res);

			res = (res + res % glm::ivec3(2)) / glm::ivec3(2);
		}

		float MSSIM = 1.0f;
		float W = 1.0f;
		float w = 1.0f;
		for (uint32_t i = 0; i < mipmapLVL; ++i)
		{
			w /= 2.0f;
			if (lcs[i].z > 0)	MSSIM *= pow(lcs[i].y * lcs[i].z, w);
			else
			{
				MSSIM *= pow(lcs[i - 1].x, W);
				break;
			}
			W -= w;
		}
		TINYVR_INFO("Get MSSIM = {0:>16.6}, SSIM = [{1:>10.6}]\n", MSSIM, fmt::join(SSIM, ","));
	}
}