#pragma once

#include "tinyVR/core/vrWindow.h"
#include "tinyVR/renderer/vrGraphicsContext.h"

struct GLFWwindow;

namespace tinyvr {

    class vrWindowsWindow : public vrWindow
    {
	public:
		vrWindowsWindow(const vrWindowProps& props);
		virtual ~vrWindowsWindow();

		void OnUpdate() override;

		unsigned int GetWidth() const override { return m_Data.Width; }
		unsigned int GetHeight() const override { return m_Data.Height; }

		// Window attributes
		void SetEventCallback(const vrWindow::EventCallbackFn& callback) override { m_Data.EventCallback = callback; }
		void SetVSync(bool enabled) override;
		bool IsVSync() const override;

		void* GetNativeWindow() const override { return m_Window; }
	private:
		virtual void Init(const vrWindowProps& props);
		virtual void Shutdown();
	private:
		GLFWwindow* m_Window;
		vrGraphicsContext* m_Context;

		struct WindowData
		{
			std::string Title = "";
			unsigned int Width = 0;
			unsigned int Height = 0;
			bool VSync = true;

			EventCallbackFn EventCallback = nullptr;
		} m_Data;
	};
}
