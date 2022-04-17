#pragma once

#include <cstddef>

#include <glm/glm.hpp>

namespace tinyvr
{
	struct tfCNode { uint32_t isoVal; glm::vec3 C; };
	struct tfANode { uint32_t isoVal; float a; };

	inline bool operator==(const tfCNode& lhs, const tfCNode& rhs) noexcept { return lhs.isoVal == rhs.isoVal; }
	inline bool operator<=(const tfCNode& lhs, const tfCNode& rhs) noexcept { return lhs.isoVal <= rhs.isoVal; }
	inline bool operator>=(const tfCNode& lhs, const tfCNode& rhs) noexcept { return lhs.isoVal >= rhs.isoVal; }
	inline bool operator<(const tfCNode& lhs, const tfCNode& rhs) noexcept { return lhs.isoVal < rhs.isoVal; }
	inline bool operator>(const tfCNode& lhs, const tfCNode& rhs) noexcept { return lhs.isoVal > rhs.isoVal; }
	inline bool operator==(const tfCNode& node, uint32_t isoVal) noexcept { return node.isoVal == isoVal; }
	inline bool operator<=(const tfCNode& node, uint32_t isoVal) noexcept { return node.isoVal <= isoVal; }
	inline bool operator>=(const tfCNode& node, uint32_t isoVal) noexcept { return node.isoVal >= isoVal; }
	inline bool operator<(const tfCNode& node, uint32_t isoVal) noexcept { return node.isoVal < isoVal; }
	inline bool operator>(const tfCNode& node, uint32_t isoVal) noexcept { return node.isoVal > isoVal; }

	inline bool operator==(const tfANode& lhs, const tfANode& rhs) noexcept { return lhs.isoVal == rhs.isoVal; }
	inline bool operator<=(const tfANode& lhs, const tfANode& rhs) noexcept { return lhs.isoVal <= rhs.isoVal; }
	inline bool operator>=(const tfANode& lhs, const tfANode& rhs) noexcept { return lhs.isoVal >= rhs.isoVal; }
	inline bool operator<(const tfANode& lhs, const tfANode& rhs) noexcept { return lhs.isoVal < rhs.isoVal; }
	inline bool operator>(const tfANode& lhs, const tfANode& rhs) noexcept { return lhs.isoVal > rhs.isoVal; }
	inline bool operator==(const tfANode& node, uint32_t isoVal) noexcept { return node.isoVal == isoVal; }
	inline bool operator<=(const tfANode& node, uint32_t isoVal) noexcept { return node.isoVal <= isoVal; }
	inline bool operator>=(const tfANode& node, uint32_t isoVal) noexcept { return node.isoVal >= isoVal; }
	inline bool operator<(const tfANode& node, uint32_t isoVal) noexcept { return node.isoVal < isoVal; }
	inline bool operator>(const tfANode& node, uint32_t isoVal) noexcept { return node.isoVal > isoVal; }
}