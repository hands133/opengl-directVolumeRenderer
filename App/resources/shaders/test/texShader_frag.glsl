#version 410 core

in vec2 v_Coord;
out vec4 FragColor;

uniform sampler2D u_Tex;

void main()
{
	FragColor = texture(u_Tex, v_Coord);
}