#pragma once

#include "vrCamera.h"

#include <glm/glm.hpp>

namespace tinyvr {

	class vrPerspectiveCamera : public vrCamera
	{
	public:
		vrPerspectiveCamera(glm::vec3 position, int width, int height, float zoom = 45.0f);

		void SetProjection(float zoom, float aspectRatio);
		
	private:
		void RecalculateViewMatrix() override;
		void RecalculateProjectionMatrix();
		void RecalculateViewProjectionMult();
	private:

		using vrCamera::m_Position;
		glm::vec3 m_Front;
		glm::vec3 m_Up;
		glm::vec3 m_Right;
		glm::vec3 m_WorldUp;

		float m_Zoom;
		float m_AspectRatio;
		float m_Near, m_Far;
	};

}
