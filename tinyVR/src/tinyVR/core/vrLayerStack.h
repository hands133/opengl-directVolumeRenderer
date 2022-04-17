#pragma once

#include "tinyVR/core/vrBase.h"
#include "vrLayer.h"

namespace tinyvr {

	class TINYVR_API vrLayerStack
	{
	public:
		vrLayerStack();
		~vrLayerStack();

		void PushLayer(vrLayer* layer);
		void PushOverlay(vrLayer* overlay);
		void PopLayer(vrLayer* layer);
		void PopOverlay(vrLayer* overlay);

		std::vector<vrLayer*>::iterator begin() { return m_Layers.begin(); }
		std::vector<vrLayer*>::iterator end() { return m_Layers.end(); }
		std::vector<vrLayer*>::reverse_iterator rbegin() { return m_Layers.rbegin(); }
		std::vector<vrLayer*>::reverse_iterator rend() { return m_Layers.rend(); }

		std::vector<vrLayer*>::const_iterator begin() const { return m_Layers.begin(); }
		std::vector<vrLayer*>::const_iterator end()	const { return m_Layers.end(); }
		std::vector<vrLayer*>::const_reverse_iterator rbegin() const { return m_Layers.rbegin(); }
		std::vector<vrLayer*>::const_reverse_iterator rend() const { return m_Layers.rend(); }

	private:
		std::vector<vrLayer*> m_Layers;
		unsigned int m_LayerInsertIndex = 0;
	};
}


