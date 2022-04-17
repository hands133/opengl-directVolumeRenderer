#pragma once

#include "tinyVR/core/vrBase.h"
#include "tinyVR/core/vrLayer.h"

#include "tinyVR/events/vrKeyEvent.h"
#include "tinyVR/events/vrMouseEvent.h"
#include "tinyVR/events/vrApplicationEvent.h"

namespace tinyvr {

	class TINYVR_API vrImGuiLayer : public vrLayer
	{
	public:
		vrImGuiLayer();
		~vrImGuiLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnEvent(vrEvent& e) override;

		void Begin();
		void End();

		void BlockEvents(bool block) { m_BlockEvents = block; }

	private:
		float m_Time;
		bool m_BlockEvents;
	};
}

