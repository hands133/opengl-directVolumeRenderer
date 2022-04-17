#version 410 core

layout(location = 0) out vec4 FragColor;

in vec3 v_Position;
uniform vec3 u_Color;

void main()
{
	FragColor = vec4(u_Color, 1.0f);
}