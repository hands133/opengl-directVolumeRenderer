#pragma once

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "base.h"

class TrackBall
{
    using pos = glm::dvec2;

public:

    TrackBall() {};
    ~TrackBall() {};

    virtual void setPressPos();
    virtual void setReleasePos();
    virtual void setCurrentPos(const pos& p);

    glm::mat4 getDragMat()  { return this->dragMat; }

protected:

    bool pressed = false;

    pos pressedPoint = pos(0.0);
    pos currentPoint = pos(0.0);

    glm::mat4 dragMat = glm::mat4(1.0);

    virtual glm::mat4 rotateMethod(const pos& p) = 0;
};

void TrackBall::setPressPos()
{
    pressed = true;
    this->pressedPoint = currentPoint;
}

void TrackBall::setReleasePos()
{
    pressed = false;
    dragMat = glm::mat4(1.0);
}

void TrackBall::setCurrentPos(const pos& p)
{
    this->currentPoint = p;
    if (pressed)    dragMat = this->rotateMethod(p);
}

#endif