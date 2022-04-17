#include "vrpch.h"
#include "vrPerspectiveCamera.h"

namespace tinyvr {

	vrPerspectiveCamera::vrPerspectiveCamera(glm::vec3 position, int width, int height, float zoom)
		: vrCamera(position), m_AspectRatio((float)width / (float)height),
		m_Zoom(zoom), m_Near(0.01f), m_Far(100.0f)
	{
		this->m_Position = position;
		this->m_Front = { 0.0f, 0.0f, -1.0f };
		this->m_Up = { 0.0f, 1.0f, 0.0f };
		this->m_WorldUp = m_Up;

		RecalculateViewMatrix();
		RecalculateProjectionMatrix();
		RecalculateViewProjectionMult();
	}

	void vrPerspectiveCamera::SetProjection(float zoom, float aspectRatio)
	{
		m_Zoom = zoom;
		m_AspectRatio = aspectRatio;
		RecalculateProjectionMatrix();
	}

	void vrPerspectiveCamera::RecalculateViewMatrix()
	{
		m_ViewMatrix = glm::lookAt(m_Position, m_Position + m_Front, m_Up);
		RecalculateViewProjectionMult();
	}

	void vrPerspectiveCamera::RecalculateProjectionMatrix()
	{
		m_ProjectionMatrix = glm::perspective(glm::radians(m_Zoom), m_AspectRatio, m_Near, m_Far);
		RecalculateViewProjectionMult();
	}

	void vrPerspectiveCamera::RecalculateViewProjectionMult()
	{
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}
}