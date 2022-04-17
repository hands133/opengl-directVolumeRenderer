#include "vrpch.h"
#include "vrLayerStack.h"

namespace tinyvr {

	vrLayerStack::vrLayerStack() {}

	vrLayerStack::~vrLayerStack()
	{
		for (auto& layer : m_Layers)	delete layer;
	}

	void vrLayerStack::PushLayer(vrLayer* layer)
	{
		m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, layer);
		m_LayerInsertIndex++;
	}

	void vrLayerStack::PushOverlay(vrLayer* overlay)
	{
		m_Layers.emplace_back(overlay);
	}

	void vrLayerStack::PopLayer(vrLayer* layer)
	{
		auto it = std::find(m_Layers.begin(), m_Layers.end(), layer);
		if (it != m_Layers.end())
		{
			m_Layers.erase(it);
			m_LayerInsertIndex--;
		}
	}

	void vrLayerStack::PopOverlay(vrLayer* overlay)
	{
		auto it = std::find(m_Layers.begin(), m_Layers.end(), overlay);
		if (it != m_Layers.end())
			m_Layers.erase(it);
	}
}