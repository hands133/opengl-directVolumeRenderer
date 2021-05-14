#pragma once

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "base.h"

class Controller
{
    using pos = glm::dvec2;

public:

    Controller() {};
    ~Controller() {};

    void setPressed();
    void setRelease();
    void setCurrent(const pos& p);

    glm::mat4 getDragMat();

private:

    bool pressed = false;

    pos pressedPoint = pos(0.0);
    pos currentPoint = pos(0.0);

    glm::mat4 dragMat = glm::mat4(1.0);

    glm::fvec3 V1;
    glm::fvec3 V2;

    static float z(const pos& p);
    static float f(float v);

    glm::mat4 rotateMethod(const pos& p);
};

void Controller::setPressed()
{
    pressed = true;
    this->pressedPoint = currentPoint;
    
    V1 = glm::vec3(pressedPoint.x, pressedPoint.y, Controller::z(pressedPoint));
    V1 = glm::normalize(V1);
}

void Controller::setRelease()
{
    pressed = false;

    dragMat = glm::mat4(1.0);
}

void Controller::setCurrent(const pos& p)
{
    this->currentPoint = p;
    if (pressed)    dragMat = this->rotateMethod(p);
}

glm::mat4 Controller::getDragMat()
{
    return this->dragMat;
}

float Controller::z(const pos& p)
{
    float d2 = glm::dot(p, p);
    float r = 1.0f;

    if (d2 < r / 2.0f)      return glm::sqrt(r * r - d2);
    else                    return r / (2.0f * glm::sqrt(d2));
}

float Controller::f(float v)
{
    if (v <= 0.0f)      return 0.0f;
    else                return glm::min(v, 1.0f) * PI_DIV2;
}

glm::mat4 Controller::rotateMethod(const pos& p)
{
    float theta = 0.0f;
    glm::vec3 rotateAxis = glm::vec3(1.0f);

    pos dir = glm::normalize(currentPoint - pressedPoint);

    V2 = glm::vec3(currentPoint.x, currentPoint.y, Controller::z(currentPoint));
    V2 = glm::normalize(V2);

    theta = glm::acos(glm::dot(V1, V2));
    rotateAxis = glm::cross(V1, V2);

    return glm::rotate(glm::mat4(1.0), theta, rotateAxis);
}

#endif