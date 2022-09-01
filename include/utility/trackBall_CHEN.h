#pragma once

#ifndef TRACKBALL_CHEN_H
#define TRACKBALL_CHEN_H

#include "trackBall.h"

class TrackBall_CHEN : public TrackBall
{
    using pos = glm::dvec2;

public:

    TrackBall_CHEN() {}
    ~TrackBall_CHEN() {}

    void setPressPos() override;
    // void setReleasePos();                // do nothing
    // void setCurrentPos(const pos& p);    // do nothing

private:

    glm::fvec3 V1, V2;

    static float z(const pos& p);
    static float f(float v);

    glm::mat4 rotateMethod(const pos& p) override;

};

float TrackBall_CHEN::z(const pos& p)
{
    float d2 = glm::dot(p, p);
    float r = 1.0f;

    if (d2 < r / 2.0f)      return glm::sqrt(r * r - d2);
    else                    return r / (2.0f * glm::sqrt(d2));
}

float TrackBall_CHEN::f(float v)
{
        if (v <= 0.0f)      return 0.0f;
        else                return glm::min(v, 1.0f) * PI_DIV2;
}

void TrackBall_CHEN::setPressPos()
{
    TrackBall::setPressPos();

    V1 = glm::vec3(pressedPoint.x, pressedPoint.y, TrackBall_CHEN::z(pressedPoint));
    V1 = glm::normalize(V1);
}

glm::mat4 TrackBall_CHEN::rotateMethod(const pos& p)
{
    float theta = 0.0f;
    glm::vec3 rotateAxis = glm::vec3(1.0f);

    pos dir = glm::normalize(currentPoint - pressedPoint);

    V2 = glm::vec3(currentPoint.x, currentPoint.y, TrackBall_CHEN::z(currentPoint));
    V2 = glm::normalize(V2);

    theta = glm::acos(glm::dot(V1, V2));
    rotateAxis = glm::cross(V1, V2);

    return glm::rotate(glm::mat4(1.0), theta, rotateAxis);
}

#endif