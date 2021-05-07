#include <glm/glm.hpp>

namespace util
{
    float calSignedAngle(glm::fvec2 src, glm::fvec2 dst)
    {
        src = glm::normalize(src);
        dst = glm::normalize(dst);

        glm::fvec3 srcV3 = { src.x, src.y, 0.0 };
        glm::fvec3 dstV3 = { dst.x, dst.y, 0.0 };
        // dot product
        float dP = glm::acos(glm::dot(srcV3, dstV3));
        // cross product
        glm::fvec3 cP = glm::cross(srcV3, dstV3);
        float sign = glm::dot(cP, dstV3);
        
        return (sign > 0) ? dP : (-dP);
    }
}