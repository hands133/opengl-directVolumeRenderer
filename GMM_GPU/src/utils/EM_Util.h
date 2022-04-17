#pragma once

#include <vector>

#include "glm/glm.hpp"

using sampStruct = glm::ivec4;

namespace EM_Util
{
    inline void ValueRange_LocateTarget(const std::vector<sampStruct>& vec, uint32_t binIdx, glm::ivec3& range)
    {
        int l = 0, r = vec.size() - 1;
        int mid = (l + r) / 2;
        int idx = -1;

        while ((l <= r) && (vec[mid].w != binIdx))
        {
            if (vec[mid].w < binIdx)   l = mid + 1;
            else                r = mid - 1;

            mid = (l + r) / 2;
        }

        if (vec[mid].w == binIdx)      idx = mid;
        range = { l, r, idx };
    }

    inline int ValueRange_LocateTargetLeft(const std::vector<sampStruct>& vec, int l, int r, uint32_t binIdx)
    {
        int mid = (l + r) / 2;
        int idx = r;

        while (l <= r)
        {
            if (vec[mid].w < binIdx)
            {
                l = mid + 1;
                mid = (l + r) / 2;
            }
            else
            {
                if ((mid > 0) && (vec[mid - 1].w == binIdx))
                {
                    r = mid - 1;
                    mid = (l + r + 1) / 2;
                }
                else if ((mid > 0) && (vec[mid - 1].w != binIdx))
                {
                    idx = mid;
                    break;
                }
                else if (mid == 0)   return 0;
            }
        }
        return idx; // left-border
    }

    inline int ValueRange_LocateTargetRight(const std::vector<sampStruct>& vec, int l, int r, uint32_t binIdx)
    {
        int mid = (l + r + 1) / 2;
        int idx = l;

        while (l <= r)
        {
            if (vec[mid].w > binIdx)
            {
                r = mid - 1;
                mid = (l + r) / 2;
            }
            else
            {
                if (((mid + 1) < vec.size()) && (vec[mid + 1].w == binIdx))
                {
                    l = mid + 1;
                    mid = (l + r) / 2;
                }
                else if (((mid + 1) < vec.size()) && (vec[mid + 1].w != binIdx))
                {
                    idx = mid;
                    break;
                }
                else if (mid == (vec.size() - 1))   return vec.size() - 1;
            }
        }
        return idx; // right-border
    }

    // find, return [l, r) else, return [-1, -1]
    inline glm::ivec2 ValueRange(const std::vector<sampStruct>& vec, uint32_t binIdx)
    {
        std::pair<int, int> range = { -1, -1 };
        if (vec.empty())    return glm::ivec2(range.first, range.second);;

        int l = 0, r = vec.size() - 1;
        int mid;

        int lBound, rBound;
        glm::ivec3 border(-1);

        ValueRange_LocateTarget(vec, binIdx, border);

        mid = border.z;
        if (mid == -1)  return glm::ivec2(range.first, range.second);
        
        l = border.x;
        r = border.y;

        lBound = ValueRange_LocateTargetLeft(vec, l, mid, binIdx);
        rBound = ValueRange_LocateTargetRight(vec, mid, r, binIdx);
        range.first = lBound;
        range.second = rBound;

        return glm::ivec2(range.first, range.second);
    }

    template <size_t D, typename Pre>
    glm::mat<D, D, Pre> KroneckerProduct(const glm::vec<D, Pre>& lhs, const glm::vec<D, Pre>& rhs)
    {
        glm::mat<D, D, Pre> result;
        for (size_t i = 0; i < D; ++i)
            for (size_t j = 0; j < D; ++j)
                result[i][j] = lhs[i] * rhs[j];

        return result;
    }
}