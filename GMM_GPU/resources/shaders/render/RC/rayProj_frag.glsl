#version 410 core

layout(location = 0) out vec4 CubeCoord;

in vec3 v_Coord;

void main()
{
	CubeCoord = vec4(v_Coord, 1.0);
}