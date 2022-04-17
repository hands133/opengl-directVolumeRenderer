#include "vrpch.h"
#include "vrTransferFunctionWidget.h"
#include "vrTransferFunction.h"

#include <imgui.h>
#include <../imgui/imgui_internal.h>
#include <glm/gtc/type_ptr.hpp>

// pMin : upper-left, pMax : lower-right
static bool IsWithin(ImVec2 pMin, ImVec2 pMax, ImVec2 P)
{
	float left = pMin.x;
	float right = pMax.x;
	float upper = pMin.y;
	float lower = pMax.y;

	if (P.x < left || P.x > right)	return false;
	if (P.y < upper || P.y > lower)	return false;

	return true;
}

namespace tinyvr
{
	vrTransferFunctionWidget::vrTransferFunctionWidget(const vrRef<vrTransferFunction>& TF, const std::string& key)
		: m_TF(TF), m_Name(key),
		m_TFParamListIdx(0), m_ControlNodeRadius(5.0f),
		m_MousePos(0, 0), m_PanelScreenPos(0, 0), m_ColorBarSpan(0.0f),
		m_ColorNodePanelHovered(false), m_AlphaNodePanelHovered(false),
		m_HoveredNClickedColorNodeIdx(-1), m_HoveredNClickedAlphaNodeIdx(-1),
		m_NodeCreationNodeType(0), m_NodeCreationIsoValue(127),
		m_NodeCreationAlpha(1.0f), m_NodeCreationColor(1.0f),
		m_NodeCreationIsoValBarActive(false), m_NodeCreationAlphaBarActive(false)
	{
		if (m_TFNameListPointer.empty())
		{
			auto& nameList = m_TF->GetTFDefaultNameList();
			m_TFNameListPointer.resize(nameList.size(), nullptr);
			for (size_t i = 0; i < nameList.size(); ++i)
				m_TFNameListPointer[i] = nameList[i].c_str();
		}

		m_ColorNodePoses.reserve(m_TF->GetColorNodes().size());
		m_AlphaNodePoses.reserve(m_TF->GetAlphaNodes().size());
	}

	void vrTransferFunctionWidget::draw(const std::vector<float>& hist, glm::vec2 histRange)
	{
		bool open = true;
		ImGui::Begin(fmt::format("{0} TF", m_Name).c_str(), &open, ImGuiWindowFlags_AlwaysAutoResize);
		m_MousePos = { ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y };

		drawTFSelectionList();
		drawHistogramUI(hist, histRange);

		drawControlNodesUI();

		ImGui::Separator();
		
		drawControlNodeCreationUI();
		
		ImGui::Separator();

		drawColorNodesListUI();
		drawAlphaNodesListUI();

		ImGui::End();

		UpdateMouseEvent();
	}

	void vrTransferFunctionWidget::drawTFSelectionList()
	{
		ImGui::Text("Parameters");
		ImGui::SameLine();
		ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth());
		if(ImGui::BeginCombo(fmt::format("##{0}_Parameters", m_Name).c_str(), m_TFNameListPointer[m_TFParamListIdx]))
		{
			for (size_t i = 0; i < m_TFNameListPointer.size(); ++i)
			{
				const char* p = m_TFNameListPointer[i];
				bool isSelected = (m_TFParamListIdx == i);
				if (ImGui::Selectable(p, isSelected))
				{
					m_TFParamListIdx = i;
					m_TF->UpdateColorParameters(i);
				}

				if (isSelected)	ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		ImGui::PopItemWidth();
	}

	void vrTransferFunctionWidget::drawHistogramUI(const std::vector<float>& hist, glm::vec2 histRange)
	{
		auto histW = ImGui::GetContentRegionAvailWidth();
		ImGui::PlotHistogram(fmt::format("##{0}_histogram", m_Name).c_str(), hist.data(), hist.size(),
			0, fmt::format("{0} Histogram", m_Name).c_str(), histRange.x, histRange.y, ImVec2(histW, 120));
		m_AlphaNodePanelHovered = ImGui::IsItemHovered();
	}

	void vrTransferFunctionWidget::drawControlNodesUI()
	{
		m_PanelScreenPos = { ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y };
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		m_PanelWidth = ImGui::GetContentRegionAvailWidth() - 4.0f;
		ImU32 White = ImColor(1.0f, 1.0f, 1.0f, 1.0f);
		ImU32 Gray = ImColor(0.5f, 0.5f, 0.5f, 0.5f);
		ImU32 Black = ImColor(0.0f, 0.0f, 0.0f, 1.0f);
		ImU32 HoveredPink = ImColor(228, 35, 236, 255);

		float m_ColorBarHeight = 20.0f;
		float x = m_PanelScreenPos.x + 4;
		float y = m_PanelScreenPos.y + 4;

		auto leftBottom = ImVec2(x, y);

		float step = 1.0 * m_PanelWidth / 256;

		auto& colorNodes = m_TF->GetColorNodes();
		auto& alphaNodes = m_TF->GetAlphaNodes();

		// draw : color bar stroke
		drawList->AddRect(ImVec2(leftBottom.x - 1, leftBottom.y - 1),
			ImVec2(leftBottom.x + m_PanelWidth + 1, leftBottom.y + m_ColorBarHeight + 1), White, 1.0f, 15, 1.5f);

		m_ColorNodePanelHovered = IsWithin(ImVec2(leftBottom.x, leftBottom.y),
			ImVec2(leftBottom.x + m_PanelWidth, leftBottom.y + m_ColorBarHeight),
			{ m_MousePos.x, m_MousePos.y });

		if (m_ColorNodePanelHovered)
		{
			uint32_t hoverColorIdx = m_MousePos.x - leftBottom.x;
			hoverColorIdx = glm::clamp(hoverColorIdx, m_TF->GetIsoValueRange().x, m_TF->GetIsoValueRange().y);
			auto C = m_TF->GetColor3(hoverColorIdx);
			ImGui::BeginTooltip();
			ImGui::Text(fmt::format("R {0:>3}, G {1:>3}, B {2:>3}",
				static_cast<int>(C.r * 255.999),
				static_cast<int>(C.g * 255.999),
				static_cast<int>(C.b * 255.999)).c_str());
			ImGui::SameLine();
			ImGui::ColorButton(fmt::format("##{0}_ColorPicker", m_Name).c_str(), { C.r, C.g, C.b, 1.0f });
			ImGui::EndTooltip();
		}

		// draw : color bar
		std::vector<glm::vec4> colors(256, glm::vec4(0.0f));
		for (int i = 0; i < 256; ++i)	colors[i] = m_TF->GetColor(i / 255.0f);
		for (int i = 0; i < 256; ++i)
		{
			auto C = colors[i];
			ImU32 col32 = ImColor(C.x, C.y, C.z, C.w);
			drawList->AddLine(ImVec2(x, y), ImVec2(x, y + m_ColorBarHeight), col32, 2.0 * step);
			x += step;
		}

		// draw : color nodes
		uint32_t CN = m_TF->GetColorNodes().size();
		m_ColorNodePoses.clear();
		for(size_t i = 0; i < CN; ++i)
		{
			auto& cNode = m_TF->GetColorNodes()[i];
			auto C = cNode.C;
			ImU32 col32 = ImColor(C.r, C.g, C.b, 1.0f);
			ImVec2 cnPos = ImVec2(m_PanelScreenPos.x + step * cNode.isoVal + 4.0, y + m_ColorBarHeight);
			m_ColorNodePoses.emplace_back(glm::vec2{ cnPos.x, cnPos.y });

			drawList->AddCircleFilled(cnPos, m_ControlNodeRadius, col32);
			drawList->AddCircle(cnPos, m_ControlNodeRadius, Black);
			if (m_HoveredNClickedColorNodeIdx == i)
				drawList->AddCircle(cnPos, m_ControlNodeRadius + 1, HoveredPink);
		}

		// draw : iso slider line
		ImColor isoSliderC = ImColor(1.0f, 1.0f, 1.0f, 0.2f);
		if (m_NodeCreationIsoValBarActive)	isoSliderC.Value.w = 0.6f;
		drawList->AddLine(ImVec2(m_PanelScreenPos.x + step * m_NodeCreationIsoValue + 4.0f, y - 128.0f),
			ImVec2(m_PanelScreenPos.x + step * m_NodeCreationIsoValue + 4.0f, y - 4), isoSliderC, 2.0f);

		// draw : iso slider node
		ImVec2 triBPos = ImVec2(m_PanelScreenPos.x + step * m_NodeCreationIsoValue + 4.0f, y + 6);
		ImVec2 triLPos = ImVec2(m_PanelScreenPos.x + step * m_NodeCreationIsoValue - 1.0f, y - 3);
		ImVec2 triRPos = ImVec2(m_PanelScreenPos.x + step * m_NodeCreationIsoValue + 9.0f, y - 3);
		ImColor isoSliderNodeC = ImColor(m_NodeCreationColor.r, m_NodeCreationColor.g, m_NodeCreationColor.b, 1.0f);
		drawList->AddTriangleFilled(triBPos, triRPos, triLPos, isoSliderNodeC);
		drawList->AddTriangle(triBPos, triRPos, triLPos, Black, 1.5f);

		// draw : alpha slider line
		ImColor alphaSliderC = ImColor(1.0f, 1.0f, 1.0f, 0.2f);
		if (m_NodeCreationAlphaBarActive)	alphaSliderC.Value.w = 0.6f;

		drawList->AddLine(ImVec2(m_PanelScreenPos.x + 4.0f, y - 8 - 120.0f * m_NodeCreationAlpha),
			ImVec2(m_PanelScreenPos.x + m_PanelWidth + 4.0f, y - 8 - 120.0f * m_NodeCreationAlpha), alphaSliderC, 1.5f);

		// draw : alpha line
		for (int i = 0; i < 255; ++i)
		{
			auto aBegin = glm::clamp(1.0f - colors[i].a, 0.0f, 1.0f);
			auto aEnd = glm::clamp(1.0f - colors[i + 1].a, 0.0f, 1.0f);
			drawList->AddLine(ImVec2(m_PanelScreenPos.x + step * i + 4.0f, y - 124.0f + aBegin * 112),
				ImVec2(m_PanelScreenPos.x + step * (i + 1) + 4.0f, y - 124.0f + aEnd * 112), White, 2);
		}

		// draw : alpha nodes
		uint32_t AN = m_TF->GetAlphaNodes().size();
		m_AlphaNodePoses.clear();
		for (size_t i = 0; i < AN; ++i)
		{
			auto& aNode = m_TF->GetAlphaNodes()[i];
			float a = 1.0f - aNode.a;
			ImU32 opacity = ImColor(1.0f - a, 1.0f - a, 1.0f - a, 1.0f - a);
			ImVec2 anPos = ImVec2(m_PanelScreenPos.x + step * aNode.isoVal + 4.0f, y - 124.0f + a * 112);
			m_AlphaNodePoses.emplace_back(glm::vec2{ anPos.x, anPos.y });

			drawList->AddCircleFilled(anPos, m_ControlNodeRadius, opacity, 24);
			drawList->AddCircle(anPos, m_ControlNodeRadius, Black, 24);

			if (m_HoveredNClickedAlphaNodeIdx == i)
				drawList->AddCircle(anPos, m_ControlNodeRadius + 1, HoveredPink);
		}

		ImGui::Dummy(ImVec2(m_PanelWidth, m_ColorBarHeight + 15.0f));
	}

	void vrTransferFunctionWidget::drawControlNodeCreationUI()
	{
		ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() / 2.0);
		ImGui::BeginGroup();

		ImGui::RadioButton(fmt::format("Color Node##{0}", m_Name).c_str(), &m_NodeCreationNodeType, 0);
		ImGui::SameLine();
		ImGui::RadioButton(fmt::format("Alpha Node##{0}", m_Name).c_str(), &m_NodeCreationNodeType, 1);

		ImU32 White = ImColor(1.0f);

		auto R = m_TF->GetIsoValueRange();
		ImGui::SliderInt(fmt::format("Iso Value##{0}", m_Name).c_str(), &m_NodeCreationIsoValue, R.x + 1, R.y - 1);
		m_NodeCreationIsoValBarActive = ImGui::IsItemHovered() || ImGui::IsItemActivated();

		if (m_NodeCreationNodeType == 0)
			ImGui::ColorEdit3(fmt::format("Color##{0}", m_Name).c_str(), glm::value_ptr(m_NodeCreationColor));
		else
		{
			ImGui::SliderFloat(fmt::format("Alpha##{0}", m_Name).c_str(), &m_NodeCreationAlpha, 0.0f, 1.0f, "%0.2f");
			m_NodeCreationAlphaBarActive = ImGui::IsItemHovered() || ImGui::IsItemActivated();
		}

		ImGui::EndGroup();
		ImGui::PopItemWidth();

		ImVec2 Size = ImGui::GetItemRectSize();

		ImGui::SameLine();

		if (ImGui::Button(fmt::format("Add Node##{0}", m_Name).c_str(), ImVec2(ImGui::GetContentRegionAvailWidth(), Size.y)))
		{
			if (m_NodeCreationNodeType == 0)	// Color Nodes
				m_TF->AddColorNode(m_NodeCreationIsoValue, m_NodeCreationColor);
			else
				m_TF->AddAlphaNode(m_NodeCreationIsoValue, m_NodeCreationAlpha);
		}
	}

	void vrTransferFunctionWidget::drawColorNodesListUI()
	{
		if (ImGui::TreeNode(fmt::format("Color Nodes##{0}", m_Name).c_str()))
		{
			ImGui::BeginGroup();
			ImGui::BeginChild(ImGui::GetID((void*)0), ImVec2(
				ImGui::GetContentRegionAvailWidth(), 120), true);
			
			for (size_t i = 0; i < m_TF->GetColorNodes().size(); ++i)
			{
				auto& cn = m_TF->GetColorNodes()[i];
				uint32_t isoVal = cn.isoVal;
				auto C = cn.C;

				ImGui::PushID(&m_TF->GetColorNodes()[i]);

				ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() / 2);
				if (ImGui::ColorEdit3(fmt::format("##{0}_Color", m_Name).c_str(), glm::value_ptr(C))) m_TF->UpdateColor3(isoVal, C);
					
				ImGui::SameLine();
				auto range = m_TF->GetIsoValueRange();
				if (isoVal == range.x || isoVal == range.y)
				{	// Use button instead, cannot be modified
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.5f, 0.5f, 0.5f, 0.5f });
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.5f, 0.5f, 0.5f, 0.5f });
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.5f, 0.5f, 0.5f, 0.5f });
					ImGui::Button(std::to_string(isoVal).c_str(), ImVec2(ImGui::GetContentRegionAvailWidth(), 0));
					ImGui::PopStyleColor(3);
				}
				else
				{
					ImGui::PushItemWidth(-30);

					uint32_t tmpIsoVal = isoVal;
					auto L = m_TF->GetColorNodes()[i - 1].isoVal + 1;
					auto R = m_TF->GetColorNodes()[i + 1].isoVal - 1;
					if (ImGui::SliderInt(fmt::format("##{0}_Isoval", m_Name).c_str(), (int*)&tmpIsoVal, L, R))
						m_TF->UpdateColorIsoValue(isoVal, tmpIsoVal);
					
					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
					if (ImGui::Button(fmt::format("X##{0}", m_Name).c_str(), ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
						m_TF->DeleteColorNode(isoVal);
					ImGui::PopStyleColor(3);

					ImGui::PopItemWidth();
				}
				ImGui::PopItemWidth();

				ImGui::PopID();
			}

			ImGui::EndChild();
			ImGui::EndGroup();
			ImGui::TreePop();
		}
	}

	void vrTransferFunctionWidget::drawAlphaNodesListUI()
	{
		auto range = m_TF->GetIsoValueRange();
		if (ImGui::TreeNode(fmt::format("Alpha Nodes##{0}", m_Name).c_str()))
		{
			int W = ImGui::GetContentRegionAvailWidth();

			ImGui::BeginGroup();
			ImGui::BeginChild(ImGui::GetID((void*)0), ImVec2(
				ImGui::GetContentRegionAvailWidth(), 120), true);

			for (size_t i = 0; i < m_TF->GetAlphaNodes().size(); ++i)
			{
				auto& an = m_TF->GetAlphaNodes()[i];
				uint32_t isoVal = an.isoVal;
				auto A = an.a;

				ImGui::PushID(&an.a);
				ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() / 2);

				float tmpA = A;
				if (ImGui::SliderFloat(fmt::format("##{0}_alpha", m_Name).c_str(), &tmpA, 0.0f, 1.0f, "%0.2f"))
					m_TF->UpdateAlpha(isoVal, tmpA);

				ImGui::SameLine();
				if (an.isoVal == range.x || an.isoVal == range.y)
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.5f, 0.5f, 0.5f, 0.5f });
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.5f, 0.5f, 0.5f, 0.5f });
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.5f, 0.5f, 0.5f, 0.5f });
					ImGui::Button(std::to_string(isoVal).c_str(), ImVec2(ImGui::GetContentRegionAvailWidth(), 0));
					ImGui::PopStyleColor(3);
				}
				else
				{
					ImGui::PushItemWidth(-30);

					uint32_t tmpIsoVal = isoVal;
					auto L = m_TF->GetAlphaNodes()[i - 1].isoVal + 1;
					auto R = m_TF->GetAlphaNodes()[i + 1].isoVal - 1;
					if (ImGui::SliderInt(fmt::format("##{0}_iso", m_Name).c_str(), (int*)&tmpIsoVal, L, R))
						m_TF->UpdateAlphaIsoValue(isoVal, tmpIsoVal);

					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
					if (ImGui::Button(fmt::format("X##{0}", m_Name).c_str(), ImVec2(ImGui::GetContentRegionAvailWidth(), 0)))
						m_TF->DeleteColorNode(isoVal);
					ImGui::PopStyleColor(3);

					ImGui::PopItemWidth();
				}

				ImGui::PopItemWidth();

				ImGui::PopID();
			}
			ImGui::EndChild();
			ImGui::EndGroup();
			ImGui::TreePop();
		}
	}

	int vrTransferFunctionWidget::SearchNearestColorNode(const std::vector<glm::vec2>& controlNodes, const glm::vec2& mousePos)
	{
		int idx = -1;
		auto min = std::min_element(controlNodes.begin(), controlNodes.end(), [&](const glm::vec2& lhs, const glm::vec2& rhs)
			{ return std::abs(mousePos.x - lhs.x) < std::abs(mousePos.x - rhs.x); });

		if (glm::abs(min->x - mousePos.x) < m_ControlNodeRadius * 2)
			return std::distance(controlNodes.begin(), min);

		return idx;
	}

	int vrTransferFunctionWidget::SearchNearestAlphaNode(const std::vector<glm::vec2>& controlNodes, const glm::vec2& mousePos)
	{
		int idx = -1;
		auto min = std::min_element(controlNodes.begin(), controlNodes.end(), [&](const glm::vec2& lhs, const glm::vec2& rhs)
			{ return glm::dot(lhs - mousePos, lhs - mousePos) < glm::dot(rhs - mousePos, rhs - mousePos); });

		if (glm::dot(*min - mousePos, *min - mousePos) < m_ControlNodeRadius * m_ControlNodeRadius * 2)
			return std::distance(controlNodes.begin(), min);

		return idx;
	}

	void vrTransferFunctionWidget::UpdateColorNodePoses(uint32_t idx, glm::vec2 DeltaMouse)
	{
		// DeltaMouse.y doesn't work
		if (DeltaMouse.x == 0)	return;

		float step = 1.0 * m_PanelWidth / 256;
		float deltaX = DeltaMouse.x;

		int currentIsoVal = m_TF->GetColorNodes()[m_HoveredNClickedColorNodeIdx].isoVal;
		int newIsoVal = currentIsoVal;
		
		if (deltaX < 0)	newIsoVal += std::floor(deltaX / step);
		else			newIsoVal += std::ceil(deltaX / step);

		int L = m_TF->GetIsoValueRange().x;
		int R = m_TF->GetIsoValueRange().y;
		newIsoVal = glm::clamp(newIsoVal, L + 1, R - 1);

		m_TF->UpdateColorIsoValue(currentIsoVal, newIsoVal);
	}

	void vrTransferFunctionWidget::UpdateAlphaNodePoses(uint32_t idx, glm::vec2 DeltaMouse)
	{
		// DeltaMouse.y doesn't work
		if (DeltaMouse.x == 0)	return;

		float step = 1.0 * m_PanelWidth / 256;
		float deltaX = DeltaMouse.x;
		float deltaY = DeltaMouse.y;

		int currentIsoVal = m_TF->GetAlphaNodes()[m_HoveredNClickedAlphaNodeIdx].isoVal;

		// 1. handle alpha
		m_TF->UpdateAlphaByIdx(m_HoveredNClickedAlphaNodeIdx, m_TF->GetAlphaNodes()[m_HoveredNClickedAlphaNodeIdx].a - deltaY / 120.0f);

		// 2. handle isovalue
		if (m_HoveredNClickedAlphaNodeIdx == 0 || m_HoveredNClickedAlphaNodeIdx == m_AlphaNodePoses.size() - 1)	return;
		
		int newIsoVal = currentIsoVal;
		if (deltaX < 0)	newIsoVal += std::floor(deltaX / step);
		else			newIsoVal += std::ceil(deltaX / step);
		
		int L = m_TF->GetIsoValueRange().x;
		int R = m_TF->GetIsoValueRange().y;
		newIsoVal = glm::clamp(newIsoVal, L + 1, R - 1);

		m_TF->UpdateAlphaIsoValue(currentIsoVal, newIsoVal);
	}

	void vrTransferFunctionWidget::UpdateMouseEvent()
	{
		if (!ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		{
			m_HoveredNClickedColorNodeIdx = m_ColorNodePanelHovered ? SearchNearestColorNode(m_ColorNodePoses, m_MousePos) : -1;
			m_HoveredNClickedAlphaNodeIdx = m_AlphaNodePanelHovered ? SearchNearestAlphaNode(m_AlphaNodePoses, m_MousePos) : -1;
		}

		if (m_HoveredNClickedColorNodeIdx != -1)
		{
			ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
			if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
			{
				glm::vec2 mouseDelta = { ImGui::GetIO().MouseDelta.x, ImGui::GetIO().MouseDelta.y };
				UpdateColorNodePoses(m_HoveredNClickedColorNodeIdx, mouseDelta);
			}
		}

		if (m_HoveredNClickedAlphaNodeIdx != -1)
		{
			ImGui::BeginTooltip();
			ImGui::Text(fmt::format("Alpha={0:>4.2}", m_TF->GetAlphaNodes()[m_HoveredNClickedAlphaNodeIdx].a).c_str());
			ImGui::EndTooltip();

			if (m_HoveredNClickedAlphaNodeIdx == 0 || m_HoveredNClickedAlphaNodeIdx == m_AlphaNodePoses.size() - 1)
				ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
			else	ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);

			if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
			{
				glm::vec2 mouseDelta = { ImGui::GetIO().MouseDelta.x, ImGui::GetIO().MouseDelta.y };
				UpdateAlphaNodePoses(m_HoveredNClickedAlphaNodeIdx, mouseDelta);
			}
		}

		if (m_HoveredNClickedColorNodeIdx == -1 && m_HoveredNClickedAlphaNodeIdx == -1)	ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
	}
}