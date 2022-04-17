#pragma once

#include "tinyVR.h"


#include "tinyVR/events/vrEvent.h"

#include "tinyVR/renderer/vrTrackBall_CHEN.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../files/model.h"

namespace iot {

	class IOTLayer : public tinyvr::vrLayer
	{
	public:
		IOTLayer(const std::string& path);

		void OnUpdate(tinyvr::vrTimestep ts) override;
		void OnEvent(tinyvr::vrEvent& e) override;
		virtual void OnImGuiRender() override;

	private:

		std::string m_Path;

		bool m_showLamp = false;

		Model myModel;

		tinyvr::vrRef<tinyvr::vrVertexArray> quadVA;
		tinyvr::vrRef<tinyvr::vrVertexBuffer> quadVB;
		tinyvr::vrRef<tinyvr::vrIndexBuffer> quadIB;

		glm::vec4 zeroFillerVec, oneFillerVec;

		tinyvr::vrRef<tinyvr::vrTexture2D> opaqueTexture, depthTexture, accumTexture, revealTexture;
		tinyvr::vrRef<tinyvr::vrFrameBuffer> opaqueFBO, transparentFBO;
		tinyvr::vrRef<tinyvr::vrShader> lightingShader, compositeShader, screenShader;

		tinyvr::vrRef<tinyvr::vrTrackBall_CHEN> m_TrackBall;

		tinyvr::vrRef<tinyvr::vrPerspectiveCameraController> m_CameraController;

		glm::uvec2 m_ScrSize;
		glm::mat4 m_ModelMat;
		glm::vec4 m_BGColor;
		glm::dvec2 m_MousePos;

		glm::vec3 lightPos = { 0.2f, 1.0f, 5.0f };

		float m_Opacity;
	};
}