#pragma once

#include "vrpch.h"

#include "tinyVR/core/vrBase.h"
#include "tinyVR/events/vrEvent.h"

namespace tinyvr {

	struct vrWindowProps
	{
		std::string Title;
		unsigned int Width;
		unsigned int Height;

		vrWindowProps(const std::string& title = "Hazel Engine",
			unsigned int width = 1280,
			unsigned int height = 720)
			: Title(title), Width(width), Height(height) {}
	};

	// Interface representing a desktop system based window
	class TINYVR_API vrWindow
	{
	public:
		using EventCallbackFn = std::function<void(vrEvent&)>;

		virtual ~vrWindow() {}

		virtual void OnUpdate() = 0;
		virtual unsigned int GetWidth() const = 0;
		virtual unsigned int GetHeight() const = 0;

		// Windows attributes
		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
		virtual void SetVSync(bool enabled) = 0;
		virtual bool IsVSync() const = 0;

		virtual void* GetNativeWindow() const = 0;

		static vrRef<vrWindow> Create(const vrWindowProps& props = vrWindowProps());
	};
}