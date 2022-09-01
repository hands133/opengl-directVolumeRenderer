#version 400 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 coordPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 coord;

void main()
{
	gl_Position = projection * view * model * vec4(aPos, 1.0);
	coord = coordPos;
}