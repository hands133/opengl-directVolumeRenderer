#include "vrpch.h"
#include "vrCamera.h"

namespace tinyvr {

	vrCamera::vrCamera(glm::vec3 position)
		: m_ViewMatrix(1.0f), m_ProjectionMatrix(1.0f), m_Position(position)
	{
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}
}