#pragma once

#include "tinyVR/renderer/vrPerspectiveCamera.h"
#include "tinyVR/core/vrTimestep.h"

#include "tinyVR/events/vrApplicationEvent.h"
#include "tinyVR/events/vrMouseEvent.h"

namespace tinyvr {

	class vrPerspectiveCameraController
	{
	public:
		vrPerspectiveCameraController(glm::vec3 pos, int width, int height);

		void OnUpdate(vrTimestep ts);
		void OnEvent(vrEvent& e);

		void OnResize(float width, float height);

		vrPerspectiveCamera& GetCamera() { return m_Camera; }
		const vrPerspectiveCamera& GetCamera() const { return m_Camera; }

		float GetZoomLevel() { return m_ZoomLevel; }
		void SetZoomLevel(float level) { m_ZoomLevel = level; CalculateView(); }
		
	private:
		void CalculateView();
		void CalculateProj();
		
		bool OnMouseScrolled(vrMouseScrolledEvent& e);
		bool OnWindowResized(vrWindowResizeEvent& e);

	private:
		float m_AspectRatio;
		float m_ZoomLevel;

		glm::vec3 m_CameraPosition = glm::vec3(0.0f);

		vrPerspectiveCamera m_Camera;
	};
}


