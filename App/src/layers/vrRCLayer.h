#pragma once

#include "tinyVR.h"

// rendering components
#include "tinyVR/renderer/vrTrackBall_CHEN.h"
#include "tinyVR/renderer/vrTransferFunctionWidget.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace rc {

	class vrRCLayer : public tinyvr::vrLayer
	{
	public:
		vrRCLayer(const std::string& datFilePath);

		void OnUpdate(tinyvr::vrTimestep ts) override;
		void OnEvent(tinyvr::vrEvent& e) override;
		virtual void OnImGuiRender() override;

	private:

		tinyvr::vrRef<tinyvr::vrVertexArray> m_CubeVA;
		tinyvr::vrRef<tinyvr::vrVertexArray> m_ScreenVA;

		tinyvr::vrFrameBufferSpecification m_FrameSpec;
		tinyvr::vrRef<tinyvr::vrFrameBuffer> m_ProjInFB, m_ProjOutFB, m_RenderFB;

		tinyvr::vrRef<tinyvr::vrTransferFunction> m_TransferFunction;
		tinyvr::vrRef<tinyvr::vrTransferFunctionWidget> m_TFWidget;
		tinyvr::vrRef<tinyvr::vrTexture3D> m_VolumeTex;
		tinyvr::vrRef<tinyvr::vrTexture1D> m_TFFuncTex;

		tinyvr::vrRef<tinyvr::vrShader> m_ProjShader, m_RCShader;

		tinyvr::vrRef<tinyvr::vrTrackBall_CHEN> m_TrackBall;

		tinyvr::vrRef<tinyvr::vrPerspectiveCameraController> m_CameraController;

		std::vector<float> m_DataHistogram;
		glm::vec2 m_HistRange;

		glm::uvec2 m_ScrSize;
		glm::fvec2 m_VolumeValSpan;
		glm::mat4 m_ModelMat;
		glm::vec4 m_BGColor;
		glm::dvec2 m_MousePos;

		bool m_ViewportFocused;
		bool m_ViewportHovered;

		bool m_RotateObject;

		glm::vec2 m_ViewportSize;

		glm::ivec3 m_DataRes;

		float m_TimeStep = 0.0f;
	};
}
