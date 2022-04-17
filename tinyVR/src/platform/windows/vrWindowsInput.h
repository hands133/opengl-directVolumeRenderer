#pragma once

#include "tinyVR/core/vrInput.h"

namespace tinyvr {

    class vrWindowsInput : public vrInput
    {
    protected:
		virtual bool IsKeyPressedImpl(int keycode) override;
		virtual bool IsMouseButtonReleasedImpl(int button) override;
		virtual bool IsMouseButtonPressedImpl(int button) override;
		virtual std::pair<float, float> GetMousePositionImpl() override;
		virtual float GetMouseXImpl() override;
		virtual float GetMouseYImpl() override;
    };
}

