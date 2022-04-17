#version 410 core

in vec3 v_Coord;
out vec4 FragCoord;

void main()
{
	FragCoord = vec4(v_Coord, 1.0);
}