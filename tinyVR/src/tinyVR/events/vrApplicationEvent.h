#pragma once

#include "vrEvent.h"

namespace tinyvr {

	class TINYVR_API vrWindowResizeEvent : public vrEvent
	{
	public:
		vrWindowResizeEvent(unsigned int width, unsigned int height)
			: m_Width(width), m_Height(height) {}

		unsigned int GetWidth() const { return m_Width; }
		unsigned int GetHeight() const { return m_Height; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "WindowResizeEvent: [" << m_Width << ", " << m_Height << "]";
			return ss.str();
		}

		EVENT_CLASS_TYPE(WindowResize);
		EVENT_CLASS_CATEGORY(EventCategoryApplication);

	private:
		unsigned int m_Width, m_Height;
	};

	class TINYVR_API vrWindowCloseEvent : public vrEvent
	{
	public:
		vrWindowCloseEvent() {}

		EVENT_CLASS_TYPE(WindowClose);
		EVENT_CLASS_CATEGORY(EventCategoryApplication);
	};
}