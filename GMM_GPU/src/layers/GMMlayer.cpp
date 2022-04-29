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

// Encode the image
void encodePNG(const std::filesystem::path& filePath, const unsigned char* rawImageData, uint32_t width, uint32_t height)
{
	unsigned error = lodepng_encode32_file(filePath.string().c_str(), rawImageData, width, height);
	TINYVR_ASSERT(!error, "ERROR {0:>12}: {1}", error, lodepng_error_text(error));
}

namespace gmm {

	uint32_t maxOctreeDepth = 0;
	constexpr uint32_t patchSize = 4;		// for grid refinement
	constexpr uint32_t stencilSize = 3;		// for localEntropy

	vrGMMLayer::vrGMMLayer(
		const std::string& gmmDataDir, glm::uvec3 brickRes,
		uint32_t blockSize, glm::uvec3 dataRes,
		glm::vec2 valueRange, uint32_t numIntervals,
		// Parameters for octree
		float rho, float e0,
		// Parameters for pic output
		const std::filesystem::path picBaseDir) :
			m_GMMBaseDir(gmmDataDir), m_BrickRes(brickRes), m_BlockSize(blockSize),
			m_DataRes(dataRes), m_ValueRange(valueRange), m_NumIntervals(numIntervals),
			m_ScrSize(1280, 720), m_ModelMat(1.0f), m_BGColor(0.32156862f, 0.34117647f, 0.43137255f, 1.0f),
			m_MousePos(0.0f), m_ViewportSize(m_ScrSize.x, m_ScrSize.y), m_GAMMA(0.3f), m_EntropyRange(0.0f),
			m_rho(rho), m_e0(e0), m_PicBaseDirPath(picBaseDir)
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

		reconVol();
		calculateHistogram();

		initRCComponent();
		initEntropyComponent();

		constructOctree();
		//reconByAMR();

		//OutputAMRReconVolume("S://SGMM Recon", true);
		//OutputAMRReconVolume("S://SGMM Recon", false);

		// Reconstruction Quality
		//measureRMSE();
		//measureNMSE();
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

			m_AMRRCShader->SetTexture("volumeTexU8", m_FullReconVolumeTex);
			m_AMRRCShader->SetTexture("entropyF32", m_LocalEntropyTex);

			if (m_VisVolOrEntropy) {
				m_AMRRCShader->SetBool("showVolOrTex", true);
				m_AMRRCShader->SetTexture("tFunc", m_TFVolume->GetTFTexture());
				m_AMRRCShader->SetFloat("vMin", m_ValueRange.x);
				m_AMRRCShader->SetFloat("vMax", m_ValueRange.y);
			}
			else {
				m_AMRRCShader->SetBool("showVolOrTex", false);
				m_AMRRCShader->SetTexture("tFunc", m_TFEntropy->GetTFTexture());
				m_AMRRCShader->SetFloat("vMin", m_EntropyRange.x);
				m_AMRRCShader->SetFloat("vMax", m_EntropyRange.y);
			}
			m_AMRRCShader->SetInt3("dataRes", m_DataRes);
			m_AMRRCShader->SetInt("NumIntervals", m_NumIntervals);

			m_AMRRCShader->SetTexture("octreeNodePool", m_TreeNodeTex);

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
			ImGui::RadioButton("Opaque", &m_SaveOpaqueOrNot, 1);
			ImGui::SameLine();
			ImGui::RadioButton("With BG", &m_SaveOpaqueOrNot, 0);

			if (ImGui::Button("Save Rendered Image As..."))
			{
				tinyvr::vrRef<tinyvr::vrTexture2D> tex;
				if (m_SaveOpaqueOrNot)	tex = m_RenderFB->GetColorAttachment(0);
				else					tex = m_DisplayFB->GetColorAttachment(0);
				
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
						uint8_t a = m_SaveOpaqueOrNot ? static_cast<uint8_t>(pixel.a * 255.999f) : 255;
						return glm::u8vec4(r, g, b, a);
					});

				std::filesystem::path picDirPath = fmt::format("{0}/{1}/{2}",
					m_PicBaseDirPath.string(),
					m_VisVolOrEntropy ? "Volume" : "Entropy",
					m_ClipVolume ? "Slice" : "RC");

				std::string picPrefixName = m_PicBaseDirPath.filename().string();
				std::string picPrefixRenderType = m_VisVolOrEntropy ? "V" : "E";
				std::string picPrefixRhoVal = fmt::format("{0}", m_rho);
				std::string picPrefixOPBG = m_SaveOpaqueOrNot ? "OP" : "BG";
				std::string picPrefixRenderClip = m_ClipVolume ? "S" : "";
				std::string picPrefixRenderClipDir = "Z";
				std::string picPrefixRenderClipPos = fmt::format("{0}", m_ClipPlane);

				if (!std::filesystem::exists(picDirPath))
					std::filesystem::create_directories(picDirPath);

				std::string saveFileName = "";
				if (m_ClipVolume)
					saveFileName = fmt::format("{0}/{1}{2}_{3}_{4}_{5}{6}_{7}",
						picDirPath.string(), picPrefixName, picPrefixRenderType, picPrefixRhoVal,
						picPrefixOPBG, picPrefixRenderClip, picPrefixRenderClipDir, picPrefixRenderClipPos);
				else
					saveFileName = fmt::format("{0}/{1}{2}_{3}_{4}",
						picDirPath.string(), picPrefixName, picPrefixRenderType, picPrefixRhoVal, picPrefixOPBG);
				if (m_ShowGrid)	saveFileName += "_G";

				saveFileName += ".png";


				for (int i = 0; i < W; ++i)
					for (int j = 0; j < H / 2; ++j)
					{
						uint32_t srcIdx = j * W + i;
						uint32_t dstIdx = (H - j - 1) * W + i;
						std::swap(imageBufferUChar[srcIdx], imageBufferUChar[dstIdx]);
					}

				encodePNG(saveFileName.c_str(), (const unsigned char*)imageBufferUChar.data(), W, H);
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
		TINYVR_ASSERT(std::filesystem::is_directory(m_GMMBaseDir), "SGMM Dir is not a directory!");
		if (!std::filesystem::exists(m_PicBaseDirPath))	std::filesystem::create_directories(m_PicBaseDirPath);

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

			TINYVR_ASSERT(m_GMMFile->read(m_GMMBaseDir.string()), "Read data Failed");
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
					for (auto& binPair : GMMBinList)
					{
						const auto& binIdx = binPair.first;
						const auto& bin = binPair.second;

						if (n >= bin.GetKernelNum())	continue;
						const auto& kernel = bin.GetKernel(n);

						// GMMBin weight & mean value
						auto idx = k * (W * D) + binIdx * W + i;
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

		m_GMMFile->ReleaseDataBuffer();
	}

	void vrGMMLayer::initVolumeTex()
	{
		TINYVR_PROFILE_FUNCTION();
		uint64_t NSamps = m_DataRes.x * m_DataRes.y * m_DataRes.z;

		m_FullReconVolumeTex = tinyvr::vrTexture3D::Create(m_DataRes.x, m_DataRes.y, m_DataRes.z,
			tinyvr::vrTextureFormat::TEXTURE_FMT_RED, tinyvr::vrTextureType::TEXTURE_TYPE_U8I);
		m_FullReconVolumeTex->SetData(NSamps);

		m_OriginDataBuffer.resize(NSamps, 0.0f);
		
		auto originalRawFilePath = m_GMMBaseDir / "unsignedcharVolume.raw";
		TINYVR_ASSERT(std::filesystem::exists(originalRawFilePath), "Original volume file Doesn't exist!");
		// For Isabel & Deep Water Impact
		std::ifstream originFile(originalRawFilePath, std::ios::binary);
		originFile.read((char*)m_OriginDataBuffer.data(), m_DataRes.x * m_DataRes.y * m_DataRes.z * sizeof(float));
		originFile.close();

		// For plane, shock lap3d
		//std::ifstream originFile(originalRawFilePath, std::ios::binary);
		//std::vector<uint8_t> originRawBuffer(m_DataRes.x * m_DataRes.y * m_DataRes.z, 0x00);
		//originFile.read((char*)originRawBuffer.data(), m_DataRes.x * m_DataRes.y * m_DataRes.z);
		//originFile.close();
		//std::copy(originRawBuffer.begin(), originRawBuffer.end(), m_OriginDataBuffer.begin());
	}

	void vrGMMLayer::reconVol()
	{
		TINYVR_PROFILE_FUNCTION();

		if (m_ReconCompShader == nullptr)
			m_ReconCompShader = tinyvr::vrShader::CreateComp("resources/shaders/compute/reconstruct3DVolume_comp.glsl");
		m_ReconCompShader->Bind();

		m_FullReconVolumeTex->BindImage(0, tinyvr::vrTexImageAccess::TEXTURE_IMAGE_ACCESS_WRITEONLY);
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
	}

	void vrGMMLayer::calculateHistogram()
	{
		//m_GlobalHistTex = tinyvr::vrTexture2D::Create(m_NumIntervals, 1,
		//	tinyvr::vrTextureFormat::TEXTURE_FMT_RED, tinyvr::vrTextureType::TEXTURE_TYPE_U32I);
		//m_GlobalHistTex->SetData(m_NumIntervals);

		//m_GlobalHistCompShader = tinyvr::vrShader::CreateComp("resources/shaders/compute/calculateGlobalHist_comp.glsl");
		//m_GlobalHistCompShader->Bind();

		//m_VolumeTex->BindUnit(0);
		//m_GlobalHistTex->BindImage(1, tinyvr::vrTexImageAccess::TEXTURE_IMAGE_ACCESS_READWRITE);

		//m_GlobalHistCompShader->SetFloat("vMin", m_ValueRange.x);
		//m_GlobalHistCompShader->SetFloat("vMax", m_ValueRange.y);
		//m_GlobalHistCompShader->SetInt("NumIntervals", m_NumIntervals);

		//std::dynamic_pointer_cast<tinyvr::vrOpenGLCompShader>(m_GlobalHistCompShader)->Compute(m_DataRes);

		//std::vector<uint32_t> uBuffer = tinyvr::vrFrameBuffer::ReadPixelsUI(m_GlobalHistTex, { 0, 0 }, { m_NumIntervals - 1, 0 });
		m_VolumeHistogram.resize(m_NumIntervals, 0);
		//std::transform(uBuffer.begin(), uBuffer.end(), m_VolumeHistogram.begin(), [&](uint32_t v)
			//{	return static_cast<float>(v) / (m_DataRes.x * m_DataRes.y * m_DataRes.z); });

		//auto mM = std::minmax_element(m_VolumeHistogram.begin(), m_VolumeHistogram.end());
		//m_VolumeHistRange = { *(mM.first), *(mM.second) };
		m_VolumeHistRange = m_ValueRange;
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
			//m_ModelMat = glm::rotate(glm::mat4(1.0f), glm::radians(-30.0f), { 1.0f, 0.0f, 0.0f }) * m_ModelMat;
			//m_ModelMat = glm::rotate(glm::mat4(1.0f), glm::radians(-30.0f), { 0.0f, 1.0f, 0.0f }) * m_ModelMat;
			//m_ModelMat = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), { 0.0f, 1.0f, 0.0f }) * m_ModelMat;
			// Deep Water Impact
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
		m_LocalEntropyTex = tinyvr::vrTexture3D::Create(m_DataRes.x, m_DataRes.y, m_DataRes.z,
			tinyvr::vrTextureFormat::TEXTURE_FMT_RED, tinyvr::vrTextureType::TEXTURE_TYPE_FLT32);
		m_LocalEntropyTex->SetData(m_DataRes.x * m_DataRes.y * m_DataRes.z, nullptr);
			
		{	// calculate entropy using local histogram
			m_EntropyLocalHistCompShader = tinyvr::vrShader::CreateComp("resources/shaders/compute/calculateEntropyPerBrick_comp.glsl");
			m_EntropyLocalHistCompShader->Bind();

			m_LocalEntropyTex->BindImage(0, tinyvr::vrTexImageAccess::TEXTURE_IMAGE_ACCESS_WRITEONLY);
			m_FullReconVolumeTex->BindUnit(1);

			m_EntropyLocalHistCompShader->SetFloat("vMin", m_ValueRange.x);
			m_EntropyLocalHistCompShader->SetFloat("vMax", m_ValueRange.y);
			m_EntropyLocalHistCompShader->SetInt("NumIntervals", m_NumIntervals);
			m_EntropyLocalHistCompShader->SetInt3("dataRes", m_DataRes);
			m_EntropyLocalHistCompShader->SetInt("SS", 5);

			for (int i = 0; i < m_BrickRes.x; ++i)
				for (int j = 0; j < m_BrickRes.y; ++j)
					for (int k = 0; k < m_BrickRes.z; ++k)
					{
						auto O = glm::ivec3(m_XGap[i], m_YGap[j], m_ZGap[k]);
						auto R = glm::ivec3(m_XGap[i + 1] - m_XGap[i],
											m_YGap[j + 1] - m_YGap[j],
											m_ZGap[k + 1] - m_ZGap[k]);

						m_EntropyLocalHistCompShader->SetInt3("O", O);
						m_EntropyLocalHistCompShader->SetInt3("R", R);

						std::dynamic_pointer_cast<tinyvr::vrOpenGLCompShader>(m_EntropyLocalHistCompShader)->Compute(R);
					}
		}

		//{
		//	m_EntropyLocalHistCompShader = tinyvr::vrShader::CreateComp("resources/shaders/compute/calculateLocalHistEntropy_comp.glsl");
		//	m_EntropyLocalHistCompShader->Bind();

		//	m_LocalEntropyTex->BindImage(0, tinyvr::vrTexImageAccess::TEXTURE_IMAGE_ACCESS_WRITEONLY);
		//	m_FullReconVolumeTex->BindUnit(1);

		//	m_EntropyLocalHistCompShader->SetInt3("res", m_DataRes);
		//	m_EntropyLocalHistCompShader->SetFloat("vMin", m_ValueRange.x);
		//	m_EntropyLocalHistCompShader->SetFloat("vMax", m_ValueRange.y);
		//	m_EntropyLocalHistCompShader->SetInt("NumIntervals", m_NumIntervals);
		//	m_EntropyLocalHistCompShader->SetInt("StencilSize", 5);

		//	std::dynamic_pointer_cast<tinyvr::vrOpenGLCompShader>(m_EntropyLocalHistCompShader)->Compute(m_DataRes);
		//}

		//m_EntopyHistogram = m_LocalEntropyTex->GetHistogram(m_NumIntervals);
		//auto mM = std::minmax_element(m_EntopyHistogram.begin(), m_EntopyHistogram.end());
		//m_EntropyHistRange = { *(mM.first), *(mM.second) };
		m_EntopyHistogram.resize(m_NumIntervals, 0);
		m_EntropyHistRange = m_EntropyRange;

		//m_EntropyRange = m_LocalEntropyTex->GlobalMinMaxVal();
		m_EntropyRange = { 0, 4.8 };
		TINYVR_CORE_INFO("Entropy range : [{0:>8.4}, {1:>8.4}]", m_EntropyRange.x, m_EntropyRange.y);
	}
	
	void vrGMMLayer::constructOctree()
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

		{	// [Node Pool]	r: [m:1] [f:1] [off_x:15, off_y:15]
			// [Pos Pool]	r: x0  g : y0  b : z0  w : node size
			m_TreeNodeTex = tinyvr::vrTexture2D::Create(treeTexWidth, treeTexHeight,
				tinyvr::vrTextureFormat::TEXTURE_FMT_RED, tinyvr::vrTextureType::TEXTURE_TYPE_U32I);
			m_TreeNodeTex->SetData(treeTexWidth * treeTexHeight);
		}

		std::vector<glm::vec2> ERangePerLayer(treeMaxDepth, glm::vec2(0));
		{
			m_AMRMinMaxEntropyRangeTex = tinyvr::vrTexture2D::Create(treeMaxDepth, 2,
				tinyvr::vrTextureFormat::TEXTURE_FMT_RED, tinyvr::vrTextureType::TEXTURE_TYPE_32I);
			m_AMRMinMaxEntropyRangeTex->SetData(treeMaxDepth * 2);

			// calculate entropy value range at each level
			auto minmaxEntropyCompShader = tinyvr::vrShader::CreateComp("resources/shaders/compute/reconstructTBAMR_EntropyRange_comp.glsl");
			minmaxEntropyCompShader->Bind();

			for (int i = 0; i < treeMaxDepth; ++i)
			{
				m_AMRMinMaxEntropyRangeTex->BindImage(0, tinyvr::vrTexImageAccess::TEXTURE_IMAGE_ACCESS_READWRITE);
				m_LocalEntropyTex->BindUnit(1);

				minmaxEntropyCompShader->SetInt("currentLevel", i);
				minmaxEntropyCompShader->SetInt("texW", treeTexWidth);
				minmaxEntropyCompShader->SetInt("PS", patchSize);

				std::dynamic_pointer_cast<tinyvr::vrOpenGLCompShader>(minmaxEntropyCompShader)->Compute({ std::pow(8, i), 8, 1 });
			}

			std::vector<int> minmaxBuffer(treeMaxDepth * 2, 0);
			m_AMRMinMaxEntropyRangeTex->GetData(minmaxBuffer.data());

			std::cout << "Entropy Range : ";
			for (int i = 0; i < treeMaxDepth; ++i)
			{
				float m = minmaxBuffer[i] / 1.0e8;
				float M = minmaxBuffer[i + treeMaxDepth] / 1.0e8;
				ERangePerLayer[i] = { m, M };
			}

			for (auto p : ERangePerLayer)	std::cout << p.x << " ";
			std::cout << "\n";
			for (auto p : ERangePerLayer)	std::cout << p.y << " ";
			std::cout << "\n";
		}

		glm::vec2 EMinMax = { 0.0f, std::log(5 * 5 * 5) };
		{
			m_ConOctreeFlagCompShader = tinyvr::vrShader::CreateComp("resources/shaders/compute/reconstructTBAMR_flag_comp.glsl");
			m_ConOctreeRefineCompShader = tinyvr::vrShader::CreateComp("resources/shaders/compute/reconstructTBAMR_refine_comp.glsl");

			std::vector<float> entropyValves(treeMaxDepth, 0.0f);

			auto eValveLevel = [&](std::vector<float>& eBuffer, float R, glm::vec2 range)
			{
				for (int i = 0; i < eBuffer.size(); ++i)
				{
					float ri = pow(R, i);
					float ei = (1.0 - ri) * EMinMax.y + ri * range.x;

					glm::vec2 Ri = ERangePerLayer[i];
					eBuffer[i] = (Ri.y - Ri.x) / (EMinMax.y - EMinMax.x) * (ei - EMinMax.x) + Ri.x;
				}
			};

			eValveLevel(entropyValves, m_rho, { m_e0, m_EntropyRange.y });

    		for (int i = 0; i < treeMaxDepth - 1; ++i)
			{
				// step 1. check & flag
				m_ConOctreeFlagCompShader->Bind();
				
				m_TreeNodeTex->BindImage(0, tinyvr::vrTexImageAccess::TEXTURE_IMAGE_ACCESS_READWRITE);
				m_LocalEntropyTex->BindUnit(1);

				m_ConOctreeFlagCompShader->SetInt("texW", treeTexWidth);
				m_ConOctreeFlagCompShader->SetInt("PS", patchSize);
				
				m_ConOctreeFlagCompShader->SetFloat("eValve", entropyValves[i]);
				m_ConOctreeFlagCompShader->SetInt("currentLevel", i);  

				std::dynamic_pointer_cast<tinyvr::vrOpenGLCompShader>(m_ConOctreeFlagCompShader)
					->Compute({ std::pow(8, i), 1, 1});

				// step 2. refinement
				m_ConOctreeRefineCompShader->Bind();

				m_TreeNodeTex->BindImage(0, tinyvr::vrTexImageAccess::TEXTURE_IMAGE_ACCESS_READWRITE);

				m_ConOctreeRefineCompShader->SetInt("texW", treeTexWidth);
				m_ConOctreeRefineCompShader->SetInt("PS", patchSize);
				m_ConOctreeRefineCompShader->SetInt("currentLevel", i);

				std::dynamic_pointer_cast<tinyvr::vrOpenGLCompShader>(m_ConOctreeRefineCompShader)
					->Compute({ std::pow(8, i), 8, 1 });
			}
			fmt::print("Entropy Param : [{0:>8.6}]\n", fmt::join(entropyValves, ","));

			// Octree Statistical
			{	
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

				uint32_t nBytesLT256 = nInnerNodes + (patchSize * patchSize * patchSize) * nLeafNodes / 4;
				uint32_t nBytesGT256 = nInnerNodes + (patchSize * patchSize * patchSize) * nLeafNodes / 2;
				uint32_t tBytesLT256 = (patchSize * patchSize * patchSize) * (nInnerNodes + nLeafNodes) / 4;
				uint32_t tBytesGT256 = (patchSize * patchSize * patchSize) * (nInnerNodes + nLeafNodes) / 2;
				// for lap3d, shock, plane, Isabel(alter), DWI(stone1), DWI(stone2)
				uint32_t nOriginBytes = m_DataRes.x * m_DataRes.y * m_DataRes.z;

				// for lap3dF32, Isabel(...), DWI(snd, tev, v02)
				//uint32_t nOriginBytes = m_DataRes.x * m_DataRes.y * m_DataRes.z * sizeof(float);

				TINYVR_INFO("Build TB-AMR Finished, with inner nodes = {0:>8},\n"
					"\tleaf nodes = {1:>8}, total {2:>8} nodes,\n"
					"\t{3:>10} uints with NumIntervals <= 256, compRatio = {4}\n"
					"\t{5:>10} uints with NumIntervals  > 256, compRatio = {6}\n"
					"\t\ttotal {7:>10} uints with NumIntervals  > 256, compRatio = {8}\n"
					"\t\ttotal {9:>10} uints with NumIntervals  > 256, compRatio = {10}\n",
					nInnerNodes, nLeafNodes, nInnerNodes + nLeafNodes, 
					nBytesLT256, 1.0f * nOriginBytes / nBytesLT256,
					nBytesGT256, 1.0f * nOriginBytes / nBytesGT256,
					tBytesLT256, 1.0f * nOriginBytes / tBytesLT256,
					tBytesGT256, 1.0f * nOriginBytes / tBytesGT256);
			}
		}
	}

	void vrGMMLayer::reconByAMR()
	{
		m_ReconVolumeByAMRTex = tinyvr::vrTexture3D::Create(m_DataRes.x, m_DataRes.y, m_DataRes.z,
			tinyvr::vrTextureFormat::TEXTURE_FMT_RED, tinyvr::vrTextureType::TEXTURE_TYPE_U8I);
		m_ReconVolumeByAMRTex->SetData(m_DataRes.x * m_DataRes.y * m_DataRes.z);

		m_CalAMRReconCompShader = tinyvr::vrShader::CreateComp("resources/shaders/compute/reconstructTBAMR_SGMM_comp.glsl");
		m_CalAMRReconCompShader->Bind();

		m_ReconVolumeByAMRTex->BindImage(0, tinyvr::vrTexImageAccess::TEXTURE_IMAGE_ACCESS_WRITEONLY);
		m_TreeNodeTex->BindUnit(1);

		for (int i = 0; i < m_GMMCoeffTexturesList.size(); ++i)	m_GMMCoeffTexturesList[i]->BindUnit(i + 2);

		m_CalAMRReconCompShader->SetInt("B", m_BlockSize);
		m_CalAMRReconCompShader->SetInt("PS", patchSize);

		m_CalAMRReconCompShader->SetInt("NumIntervals", m_NumIntervals);
		m_CalAMRReconCompShader->SetInt("NumBricks", m_BrickRes.x * m_BrickRes.y * m_BrickRes.z);

		m_CalAMRReconCompShader->SetInt3("BrickRes", m_BrickRes);
		m_CalAMRReconCompShader->SetInt3("DataRes", m_DataRes);

		m_CalAMRReconCompShader->SetInts("XGap", m_XGap, m_BrickRes.x + 1);
		m_CalAMRReconCompShader->SetInts("YGap", m_YGap, m_BrickRes.y + 1);
		m_CalAMRReconCompShader->SetInts("ZGap", m_ZGap, m_BrickRes.z + 1);

		m_CalAMRReconCompShader->SetInts("XBlockGap", m_XBlockGap, m_BrickRes.x + 1);
		m_CalAMRReconCompShader->SetInts("YBlockGap", m_YBlockGap, m_BrickRes.y + 1);
		m_CalAMRReconCompShader->SetInts("ZBlockGap", m_ZBlockGap, m_BrickRes.z + 1);

		for (int i = 0; i < m_BrickRes.x; ++i)
			for (int j = 0; j < m_BrickRes.y; ++j)
				for (int k = 0; k < m_BrickRes.z; ++k)
				{
					auto O = glm::ivec3(m_XGap[i], m_YGap[j], m_ZGap[k]);
					auto R = glm::ivec3(m_XGap[i + 1], m_YGap[j + 1], m_ZGap[k + 1]) - O;

					m_CalAMRReconCompShader->SetInt3("O", O);
					m_CalAMRReconCompShader->SetInt3("R", R);
					m_CalAMRReconCompShader->SetInt("BRICK_IDX", k * m_BrickRes.y * m_BrickRes.x + j * m_BrickRes.x + i);

					std::dynamic_pointer_cast<tinyvr::vrOpenGLCompShader>(m_CalAMRReconCompShader)->Compute(R);
				}
	}

	void gmm::vrGMMLayer::measureRMSE()
	{
		std::vector<uint8_t> reconAMRBuffer(m_DataRes.x * m_DataRes.y * m_DataRes.z, 0.0f);
		m_ReconVolumeByAMRTex->GetData(reconAMRBuffer.data());

		double MSE = 0.0;
		double dv = (m_ValueRange.y - m_ValueRange.x) / m_NumIntervals;
		for (uint64_t i = 0; i < m_DataRes.x * m_DataRes.y * m_DataRes.z; ++i)
		{
			float v1 = m_OriginDataBuffer[i];
			int b2 = reconAMRBuffer[i];
			float v2 = m_ValueRange.x + b2 * dv;
			float dv = v1 - v2;
			MSE += (dv * dv);
		}
		MSE /= (m_DataRes.x * m_DataRes.y * m_DataRes.z);
		double RMSE = std::sqrt(MSE);

		TINYVR_INFO("Calculate RMSE Finished, RMSE = {1:>10.6}", MSE, RMSE);
	}

	void gmm::vrGMMLayer::measureNMSE()
	{
		std::vector<uint8_t> reconAMRBuffer(m_DataRes.x * m_DataRes.y * m_DataRes.z, 0.0f);
		m_ReconVolumeByAMRTex->GetData(reconAMRBuffer.data());

		double nominator = 0.0, denominator = 0.0;
		double dv = (m_ValueRange.y - m_ValueRange.x) / m_NumIntervals;
		for (uint64_t i = 0; i < m_DataRes.x * m_DataRes.y * m_DataRes.z; ++i)
		{
			float v1 = m_OriginDataBuffer[i];
			int b2 = reconAMRBuffer[i];
			float v2 = m_ValueRange.x + b2 * dv;
			float dv = v1 - v2;
			nominator += (dv * dv);
			denominator += (v1 * v1);
		}
		double NMSE = nominator / denominator;

		TINYVR_INFO("Calculate RMSE Finished, NMSE = {0:>10.6}", NMSE);
	}

	void gmm::vrGMMLayer::OutputAMRReconVolume(const std::filesystem::path& dataDir, bool isFullReconVolume)
	{
		std::filesystem::path dataDirPath = fmt::format("{0}/{1}",
			dataDir.string(),
			m_VisVolOrEntropy ? "Volume" : "Entropy");

		std::string picPrefixName = m_PicBaseDirPath.filename().string();

		dataDirPath = dataDirPath / picPrefixName;
		if (!std::filesystem::exists(dataDirPath))
			std::filesystem::create_directories(dataDirPath);

		tinyvr::vrRef<tinyvr::vrTexture3D> targetTexture;
		std::string saveFileName = "";
		
		if (isFullReconVolume)
		{
			saveFileName = fmt::format("{0}_{1}.raw", picPrefixName, "FullRecon");
			targetTexture = m_FullReconVolumeTex;
		}
		else
		{
			saveFileName = fmt::format("{0}_{1}_{2}_{3}.raw", 
				picPrefixName, 
				isFullReconVolume ? "FullRecon" : "AMRRecon",
				m_e0, m_rho);
			targetTexture = m_ReconVolumeByAMRTex;
		}

		std::filesystem::path targetFilePath = dataDirPath / saveFileName;
		std::ofstream file(targetFilePath, std::ios::binary);

		std::vector<float> buffer(m_DataRes.x * m_DataRes.y * m_DataRes.z, 0.0f);

		double dv = (m_ValueRange.y - m_ValueRange.x) / m_NumIntervals;
		std::vector<uint8_t> fullReconUCharBuffer(m_DataRes.x * m_DataRes.y * m_DataRes.z);
		targetTexture->GetData(fullReconUCharBuffer.data());
		std::transform(fullReconUCharBuffer.begin(), fullReconUCharBuffer.end(), buffer.begin(),
			[&](uint8_t idx) { return m_ValueRange.x + static_cast<int>(idx) * dv; });

		file.write((const char*)buffer.data(), m_DataRes.x * m_DataRes.y * m_DataRes.z * sizeof(float));
		file.close();

		TINYVR_INFO("Write Raw Data {0}", targetFilePath.string());
	}
}