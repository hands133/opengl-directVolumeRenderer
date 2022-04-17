#pragma once

#include "vrTransferFunction.h"

namespace tinyvr
{
	class vrTransferFunctionWidget
	{
	public:
		static vrRef<vrTransferFunctionWidget> Create(const vrRef<vrTransferFunction>& TF, const std::string& key)
		{
			return std::make_shared<vrTransferFunctionWidget>(TF, key);
		}

		vrTransferFunctionWidget(const vrRef<vrTransferFunction>& TF, const std::string& key);

		void draw(const std::vector<float>& hist, glm::vec2 histRange);

	private:
		void drawTFSelectionList();
		void drawHistogramUI(const std::vector<float>& hist, glm::vec2 histRange);
		void drawControlNodesUI();
		void drawControlNodeCreationUI();
		void drawColorNodesListUI();
		void drawAlphaNodesListUI();

		int SearchNearestColorNode(const std::vector<glm::vec2>& controlNodes, const glm::vec2& mousePos);
		int SearchNearestAlphaNode(const std::vector<glm::vec2>& controlNodes, const glm::vec2& mousePos);

		void UpdateColorNodePoses(uint32_t idx = 0, glm::vec2 DeltaMouse = { 0, 0 });
		void UpdateAlphaNodePoses(uint32_t idx = 0, glm::vec2 DeltaMouse = { 0, 0 });

		void UpdateMouseEvent();

	private:
		vrRef<vrTransferFunction> m_TF;
		std::string m_Name;

		std::vector<const char*> m_TFNameListPointer;	// for list box
		int m_TFParamListIdx;
		float m_ControlNodeRadius;

		// Mouse-Control Component
		glm::vec2 m_MousePos;
		glm::vec2 m_PanelScreenPos;
		glm::vec4 m_ColorBarSpan;
		bool m_ColorNodePanelHovered;
		bool m_AlphaNodePanelHovered;
		int m_HoveredNClickedColorNodeIdx;
		int m_HoveredNClickedAlphaNodeIdx;
		std::vector<glm::vec2> m_ColorNodePoses, m_AlphaNodePoses;

		float m_PanelWidth = 0.0f;
		float m_ColorBarHeight = 20.0f;
		
		// Control Nodes Creation Component
		int m_NodeCreationNodeType;
		int m_NodeCreationIsoValue;
		float m_NodeCreationAlpha;
		glm::vec3 m_NodeCreationColor;

		bool m_NodeCreationIsoValBarActive;
		bool m_NodeCreationAlphaBarActive;
	};
}
