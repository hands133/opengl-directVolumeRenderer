#version 400 core

in vec3 coord;
out vec4 FragColor;

void main()
{
	FragColor = vec4(coord, 1.0);
}