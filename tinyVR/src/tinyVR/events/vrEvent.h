#pragma once

#include "tinyVR/core/vrBase.h"

#include <string>
#include <functional>
#include <sstream>

#include "spdlog/fmt/fmt.h"
#include "spdlog/fmt/ostr.h"

namespace tinyvr {

	enum class EventType
	{
		None = 0,
		WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
		//AppTick, AppUpdate, AppRender,	// useless currently
		KeyPressed, KeyReleased, KeyTyped,
		MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
	};

	enum EventCategory
	{
		None = 0,
		EventCategoryApplication =		BIT(0),
		EventCategoryInput =			BIT(1),
		EventCategoryKeyboard =			BIT(2),
		EventCategoryMouse =			BIT(3),
		EventCategoryMouseButton =		BIT(4)
	};


#define EVENT_CLASS_TYPE(type) static EventType GetStaticType() { return EventType::##type; }\
								virtual EventType GetEventType() const override { return GetStaticType(); }\
								virtual const char* GetName() const override { return #type; }

#define EVENT_CLASS_CATEGORY(category) virtual int GetCategoryFlags() const override { return category; }


	// basic class for event
	class TINYVR_API vrEvent
	{
		friend class vrEventDispatcher;

	public:

		virtual ~vrEvent() {}

		bool Handled = false;

		virtual EventType GetEventType() const = 0;
		virtual const char* GetName() const = 0;
		virtual int GetCategoryFlags() const = 0;
		virtual std::string ToString() const { return std::string(GetName()); }

		bool IsInCategory(EventCategory category) { return GetCategoryFlags() & category; }

	protected:
		bool m_Handled = false;
	};

	class vrEventDispatcher
	{
		template <typename T>
		using vrEventFn = std::function<bool(T&)>;
	public:
		vrEventDispatcher(vrEvent& event) : m_Event(event) {}

		// F will be deduced by the compiler
		template <typename T, typename F>
		bool Dispatch(const F& func)
		{
			if (m_Event.GetEventType() == T::GetStaticType())
			{
				m_Event.Handled |= func(static_cast<T&>(m_Event));
				return true;
			}
			return false;
		}

	private:
		vrEvent& m_Event;
	};

	inline std::ostream& operator<<(std::ostream& os, const vrEvent& e)
	{
		return os << e.ToString() << std::string("\n");
	}
}

namespace fmt
{
	template<>
	struct fmt::formatter<tinyvr::vrEvent>
	{
		template <typename ParseContext>
		constexpr auto parse(ParseContext& ctx) { return ctx.begin(); }

		template <typename FormatContext>
		auto format(const tinyvr::vrEvent& e, FormatContext& ctx)
		{
			return format_to(ctx.out(), e.ToString());
		}
	};
}