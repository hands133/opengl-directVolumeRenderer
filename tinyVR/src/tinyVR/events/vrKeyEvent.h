#pragma once

#include "vrEvent.h"

namespace tinyvr {

	class TINYVR_API vrKeyEvent : public vrEvent
	{
	public:
		int GetKeyCode() const { return m_KeyCode; }

		EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput);

	protected:
		vrKeyEvent(int keycode) : m_KeyCode(keycode) {}

		int m_KeyCode;
	};

	class TINYVR_API vrKeyPressedEvent : public vrKeyEvent
	{
	public:
		vrKeyPressedEvent(int keycode, int repeatCount)
			: vrKeyEvent(keycode), m_RepeatCount(repeatCount) {}

		int GetRepeatCount() const { return m_RepeatCount; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "KeyPressedEvent: " << m_KeyCode << " (" << m_RepeatCount << " repeats)";
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyPressed);
	protected:
		int m_RepeatCount;
	};

	class TINYVR_API vrKeyReleasedEvent : public vrKeyEvent
	{
	public:
		vrKeyReleasedEvent(int keycode) : vrKeyEvent(keycode) {}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "KeyReleasedEvent: " << m_KeyCode;
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyReleased);
	};

	class TINYVR_API vrKeyTypedEvent : public vrKeyEvent
	{
	public:
		vrKeyTypedEvent(int keycode)
			: vrKeyEvent(keycode) {}

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "KeyTypedEvent: " << m_KeyCode;
			return ss.str();
		}

		EVENT_CLASS_TYPE(KeyTyped);
	};
}

