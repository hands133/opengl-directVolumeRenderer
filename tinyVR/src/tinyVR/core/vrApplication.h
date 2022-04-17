#pragma once


#include "tinyVR/core/vrBase.h"

#include "vrWindow.h"
#include "vrLayerStack.h"
#include "tinyVR/events/vrEvent.h"

#include "tinyVR/imGui/vrImGuiLayer.h"

namespace tinyvr {

	class vrWindowCloseEvent;

	class TINYVR_API vrApplication
	{
	public:
		vrApplication(const std::string& name = "tinyVR App");
		virtual ~vrApplication();

		void Run();

		void OnEvent(vrEvent& e);

		// Layer operations
		void PushLayer(vrLayer* layer);
		void PushOverlay(vrLayer* overlay);

		vrWindow& GetWindow() { return *m_Window; }

		void Close() { m_Running = false; }
		vrImGuiLayer* GetImGuiLayer() { return m_ImGuiLayer; }

		static vrApplication& Get() { return *s_Instance; }
	private:
		bool OnWindowClose(vrWindowCloseEvent& e);
		bool OnWindowResize(vrWindowResizeEvent& e);

	private:
		vrRef<vrWindow> m_Window;
		vrImGuiLayer* m_ImGuiLayer;
		vrLayerStack m_LayerStacks;

		bool m_Running = true;
		bool m_Minimized = false;
		float m_LastFrameTime = 0.0f;

	private:
		static vrApplication* s_Instance;
	};

	// To be defined in CLIENT
	vrApplication* CreateApplication();
}


