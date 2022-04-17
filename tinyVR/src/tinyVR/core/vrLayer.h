#pragma once

#include "tinyVR/core/vrBase.h"
#include "tinyVR/events/vrEvent.h"
#include "tinyVR/core/vrTimestep.h"

namespace tinyvr {

	class TINYVR_API vrLayer
	{
	public:
		vrLayer(const std::string& name = "Layer");
		virtual ~vrLayer();

		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnUpdate(vrTimestep ts) {}
		virtual void OnImGuiRender() {}
		virtual void OnEvent(vrEvent& e) {}

		const std::string& GetName() const { return m_DebugName; }

	protected:
		std::string m_DebugName;
	};
}

