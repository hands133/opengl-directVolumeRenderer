#include "vrpch.h"
#include "vrWindowsInput.h"

#include "tinyVR/core/vrApplication.h"
#include <GLFW/glfw3.h>

namespace tinyvr {

	vrInput* vrInput::s_Instance = new vrWindowsInput();

	bool vrWindowsInput::IsKeyPressedImpl(int keycode)
	{
		auto window = static_cast<GLFWwindow*>(vrApplication::Get().GetWindow().GetNativeWindow());
		auto state = glfwGetKey(window, keycode);
		return (state == GLFW_PRESS) || (state == GLFW_REPEAT);
	}

	bool vrWindowsInput::IsMouseButtonPressedImpl(int button)
	{
		auto window = static_cast<GLFWwindow*>(vrApplication::Get().GetWindow().GetNativeWindow());
		auto state = glfwGetMouseButton(window, button);
		return state == GLFW_PRESS;
	}

	bool vrWindowsInput::IsMouseButtonReleasedImpl(int button)
	{
		auto window = static_cast<GLFWwindow*>(vrApplication::Get().GetWindow().GetNativeWindow());
		auto state = glfwGetMouseButton(window, button);
		return state == GLFW_RELEASE;
	}

	std::pair<float, float> vrWindowsInput::GetMousePositionImpl()
	{
		auto window = static_cast<GLFWwindow*>(vrApplication::Get().GetWindow().GetNativeWindow());
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);

		return { static_cast<float>(xpos), static_cast<float>(ypos) };
	}

	float vrWindowsInput::GetMouseXImpl()
	{
		auto [x, y] = GetMousePositionImpl();		// C++17
		return x;
	}

	float vrWindowsInput::GetMouseYImpl()
	{
		auto [x, y] = GetMousePositionImpl();		// C++17
		return x;
	}
}