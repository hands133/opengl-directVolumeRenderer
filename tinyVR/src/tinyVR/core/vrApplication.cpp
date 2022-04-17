#include "vrpch.h"
#include "vrApplication.h"

#include "tinyVR/core/vrLog.h"
#include "tinyVR/events/vrApplicationEvent.h"

#include "tinyVR/renderer/vrRenderer.h"

#include "vrInput.h"

#include <GLFW/glfw3.h>

namespace tinyvr {

	vrApplication* vrApplication::s_Instance = nullptr;

	vrApplication::vrApplication(const std::string& name)
	{
		TINYVR_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;

		m_Window = vrWindow::Create(vrWindowProps(name));
		m_Window->SetEventCallback(TINYVR_BIND_EVENT_FN(vrApplication::OnEvent));

		vrRenderer::Init();

		m_ImGuiLayer = new vrImGuiLayer();
		PushOverlay(m_ImGuiLayer);
	}

	vrApplication::~vrApplication()
	{
		vrRenderer::Shutdown();
	}

	void vrApplication::OnEvent(vrEvent& e)
	{
		vrEventDispatcher dispacher(e);
		dispacher.Dispatch<vrWindowCloseEvent>(TINYVR_BIND_EVENT_FN(vrApplication::OnWindowClose));
		dispacher.Dispatch<vrWindowResizeEvent>(TINYVR_BIND_EVENT_FN(vrApplication::OnWindowResize));

		for (auto it = m_LayerStacks.rbegin(); it != m_LayerStacks.rend(); ++it)
		{
			if (e.Handled)	break;
			(*it)->OnEvent(e);
		}
	}

	void vrApplication::PushLayer(vrLayer* layer)
	{
		m_LayerStacks.PushLayer(layer);
		layer->OnAttach();
	}

	void vrApplication::PushOverlay(vrLayer* overlay)
	{
		m_LayerStacks.PushOverlay(overlay);
		overlay->OnAttach();
	}

	void vrApplication::Run()
	{
		while (m_Running)
		{
			float time = static_cast<float>(glfwGetTime());
			vrTimestep timestep = time - m_LastFrameTime;
			m_LastFrameTime = time;

			timestep.SetTimeEpoch(time);

			if (!m_Minimized)
			{
				{
					for (auto& layer : m_LayerStacks)
						layer->OnUpdate(timestep);
				}

				m_ImGuiLayer->Begin();
				{
					for (auto& layer : m_LayerStacks)
						layer->OnImGuiRender();
				}
				m_ImGuiLayer->End();
			}
			m_Window->OnUpdate();
		}
	}

	bool vrApplication::OnWindowClose(vrWindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}

	bool vrApplication::OnWindowResize(vrWindowResizeEvent& e)
	{
		if (e.GetWidth() == 0 || e.GetHeight() == 0)
		{
			m_Minimized = true;
			return false;
		}

		m_Minimized = false;
		vrRenderer::OnWindowResize(e.GetWidth(), e.GetHeight());

		return false;
	}
}