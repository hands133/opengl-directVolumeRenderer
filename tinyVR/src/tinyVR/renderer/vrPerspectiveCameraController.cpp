#include "vrpch.h"
#include "vrPerspectiveCameraController.h"

#include "tinyVR/core/vrInput.h"
#include "tinyVR/core/vrKeyCodes.h"

namespace tinyvr {

	vrPerspectiveCameraController::vrPerspectiveCameraController(glm::vec3 pos, int width, int height)
		: m_AspectRatio(static_cast<float>(width) / static_cast<float>(height)),
		//m_ZoomLevel(45.0f),
		m_ZoomLevel(10.0f),
		m_CameraPosition(pos), m_Camera(pos, width, height, m_ZoomLevel)
	{
	}

	void vrPerspectiveCameraController::OnUpdate(vrTimestep ts)
	{
		// Control camera by keyboards and inputs
	}

	void vrPerspectiveCameraController::OnEvent(vrEvent& e)
	{
		vrEventDispatcher dispatcher(e);
		dispatcher.Dispatch<vrMouseScrolledEvent>(TINYVR_BIND_EVENT_FN(vrPerspectiveCameraController::OnMouseScrolled));
		//dispatcher.Dispatch<vrWindowResizeEvent>(TINYVR_BIND_EVENT_FN(vrPerspectiveCameraController::OnWindowResized));
	}

	void vrPerspectiveCameraController::OnResize(float width, float height)
	{
		m_AspectRatio = width / height;
		CalculateProj();
	}

	void vrPerspectiveCameraController::CalculateView()
	{
		m_Camera.SetPosition(m_CameraPosition);
	}

	void vrPerspectiveCameraController::CalculateProj()
	{
		m_Camera.SetProjection(m_ZoomLevel, m_AspectRatio);
	}

	bool vrPerspectiveCameraController::OnMouseScrolled(vrMouseScrolledEvent& e)
	{
		//m_ZoomLevel -= e.GetYOffset() * 0.2;
		m_ZoomLevel -= e.GetYOffset() * 0.1;

		m_ZoomLevel = std::max(1.0f, m_ZoomLevel);
		m_ZoomLevel = std::min(60.0f, m_ZoomLevel);

		CalculateProj();
		return false;
	}

	bool vrPerspectiveCameraController::OnWindowResized(vrWindowResizeEvent& e)
	{
		OnResize((float)e.GetWidth(), (float)e.GetHeight());
		return false;
	}
}