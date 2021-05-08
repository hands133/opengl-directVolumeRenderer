#pragma once

#include <glm/glm.hpp>

#include <string>

#ifndef DS_POD_H
#define DS_POD_H

struct Vertex
{
    glm::vec3 P;        // position, (x, y, z)
    glm::vec3 T;        // texture coordinates, for 3D texture (u, v, w)
};

#endif