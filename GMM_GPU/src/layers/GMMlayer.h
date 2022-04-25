#pragma once

#include "tinyVR.h"

#include "imgui/imgui.h"

#include "tinyVR/renderer/vrTrackBall_CHEN.h"

#include <thread>

namespace gmm
{
	class GMMFile;
}

namespace gmm {

	class vrGMMLayer : public tinyvr::vrLayer
	{
	public:
		vrGMMLayer(
			const std::string& gmmDataDir, glm::uvec3 brickRes,
			uint32_t blockSize, glm::uvec3 dataRes,
			glm::vec2 valueRange, uint32_t numIntervals,
			// Parameters for octree
			float rho, float e0,
			// Parameters for pic output
			const std::filesystem::path picBaseDir);

		~vrGMMLayer();

		void OnUpdate(tinyvr::vrTimestep ts) override;
		void OnEvent(tinyvr::vrEvent& e) override;
		virtual void OnImGuiRender() override;

	private:
		// aux member function for initialization
		// 1. Volume Reconstruction Member functions
		void initParam();
		void initGMMFile();

		void initRCComponent();

		void initVolumeTex();
		void reconVol();
		void calculateHistogram();

		// 2. Entropy Calculation functions
		void initEntropyComponent();
		// 3. Construct Local Entropy Using Valve value
		void constructOctree();
		void reconByAMR();
		// 4. Reconstruct Similarity / RMSE measure
		void measureRMSE();
		void measureNMSE();

		void OutputAMRReconVolume(const std::filesystem::path& dataDir, bool isFullReconVolume);

	private:
		/// ============ member for data ============
		std::filesystem::path m_GMMBaseDir;
		glm::uvec3 m_BrickRes;
		uint32_t m_BlockSize;

		glm::uvec3 m_DataRes;

		glm::vec2 m_ValueRange;
		glm::vec2 m_EntropyRange;
		uint32_t m_NumIntervals;

		tinyvr::vrRef<gmm::GMMFile> m_GMMFile;

		tinyvr::vrRef<tinyvr::vrTexture3D> m_OriginVolumeTex;
		std::vector<float> m_OriginDataBuffer;

		/// ============ member for controlling ============
		int32_t m_UserDefinedMaxTreeDepth;

		glm::vec4 m_GridColor;
		bool m_VisVolOrEntropy;
		bool m_ShowGrid;
		bool m_ShowPatch;
		bool m_ClipVolume;
		float m_ClipPlane = 0.0f;

		float m_timeStep;
		bool m_ViewportFocused;
		bool m_ViewportHovered;

		/// ============ member for computing ============
		//	1. Volume Reconstruction using Spatial GMM data
		std::vector<tinyvr::vrRef<tinyvr::vrTexture3D>> m_GMMCoeffTexturesList;
		tinyvr::vrRef<tinyvr::vrShader> m_ReconCompShader;

		int *m_XGap, *m_YGap, *m_ZGap;
		int *m_XBlockGap, *m_YBlockGap, *m_ZBlockGap;

		//  2. Reconstruct Local Entropy
		tinyvr::vrRef<tinyvr::vrTexture3D> m_LocalEntropyTex;
		tinyvr::vrRef<tinyvr::vrShader> m_EntropyLocalHistCompShader;

		//  3. Reconstruct Octree
		uint32_t treeMaxDepth;
		uint32_t treeTexWidth = 32768;
		uint32_t treeTexHeight;
		float m_rho;
		float m_e0;

		tinyvr::vrRef<tinyvr::vrTexture2D> m_TreePosTex, m_TreeNodeTex;
		tinyvr::vrRef<tinyvr::vrShader> m_ConOctreeFlagCompShader;
		tinyvr::vrRef<tinyvr::vrShader> m_ConOctreeRefineCompShader;

		// 4. AMR Reconstruction Result Comparison
		tinyvr::vrRef<tinyvr::vrShader> m_CalAMRNodesCompShader;
		tinyvr::vrRef<tinyvr::vrShader> m_CalAMRReconCompShader;
		tinyvr::vrRef<tinyvr::vrTexture2D> m_AMRMinMaxEntropyRangeTex;

		tinyvr::vrRef<tinyvr::vrTexture3D> m_ReconVolumeByAMRTex;

		/// ============ member for rendering ============
		tinyvr::vrRef<tinyvr::vrTrackBall_CHEN> m_TrackBall;
		tinyvr::vrRef<tinyvr::vrPerspectiveCameraController> m_CameraController;

		tinyvr::vrRef<tinyvr::vrTexture2D> m_GlobalHistTex;
		tinyvr::vrRef<tinyvr::vrShader> m_GlobalHistCompShader;

		std::vector<float> m_VolumeHistogram;
		std::vector<float> m_EntopyHistogram;
		glm::vec2 m_VolumeHistRange, m_EntropyHistRange;

		tinyvr::vrRef<tinyvr::vrTexture3D> m_VolumeTex;
		tinyvr::vrRef<tinyvr::vrTexture1D> m_TFVolumeTex, m_TFEntroyTex;
		tinyvr::vrRef<tinyvr::vrTransferFunction> m_TFVolume, m_TFEntropy;
		tinyvr::vrRef<tinyvr::vrTransferFunctionWidget> m_TFVolumeWidget, m_TFEntropyWidget;

		tinyvr::vrRef<tinyvr::vrShader> m_AMRRCShader, m_DisplayShader;
		tinyvr::vrRef<tinyvr::vrShader> m_ProjShader, m_RCShader;
		tinyvr::vrRef<tinyvr::vrVertexArray> m_CubeVA, m_DisplayVA;

		tinyvr::vrRef<tinyvr::vrFrameBuffer> m_ProjInFB, m_ProjOutFB;
		tinyvr::vrRef<tinyvr::vrFrameBuffer> m_RenderFB, m_DisplayFB;

		/// ============ member for IO ============
		std::filesystem::path m_PicBaseDirPath;
		int m_SaveOpaqueOrNot = 0;
		/// ============ member for system ============
		glm::uvec2 m_ScrSize;
		glm::mat4 m_ModelMat;
		glm::vec4 m_BGColor;
		glm::dvec2 m_MousePos;

		glm::vec2 m_ViewportSize;

		float m_GAMMA;
	};
}