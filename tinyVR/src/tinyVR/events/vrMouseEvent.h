#pragma once

#include "vrEvent.h"

namespace tinyvr {

	class TINYVR_API vrMouseMovedEvent : public vrEvent
	{
	public:
		vrMouseMovedEvent(float x, float y) : m_MouseX(x), m_MouseY(y) {}

		float GetX() const { return m_MouseX; }
		float GetY() const { return m_MouseY; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseMovedEvent: [" << m_MouseX << ", " << m_MouseY << "]";
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseMoved);
		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput);

	private:
		float m_MouseX, m_MouseY;
	};

	class TINYVR_API vrMouseScrolledEvent : public vrEvent
	{
	public:
		vrMouseScrolledEvent(float xOffset, float yOffset)
			: m_XOffset(xOffset), m_YOffset(yOffset) {}

		float GetXOffset() const { return m_XOffset; }
		float GetYOffset() const { return m_YOffset; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseScrolledEvent: [" << m_XOffset << ", " << m_YOffset << "]";
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseScrolled);
		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput);

	private:
		float m_XOffset, m_YOffset;
	};

	class TINYVR_API vrMouseButtonEvent : public vrEvent
	{
	public:
		int GetMouseButton() const { return m_Button; }

		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput);

	protected:
		vrMouseButtonEvent(int button) : m_Button(button) {}
		int m_Button;
	};

	class TINYVR_API vrMouseButtonPressedEvent : public vrMouseButtonEvent
	{
	public:
		vrMouseButtonPressedEvent(int button) : vrMouseButtonEvent(button) {};

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseButtonPressedEvent: " << m_Button;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseButtonPressed);

	};

	class TINYVR_API vrMouseButtonReleasedEvent : public vrMouseButtonEvent
	{
	public:
		vrMouseButtonReleasedEvent(int button) : vrMouseButtonEvent(button) {};

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "MouseButtonReleasedEvent: " << m_Button;
			return ss.str();
		}

		EVENT_CLASS_TYPE(MouseButtonReleased);
	};
}