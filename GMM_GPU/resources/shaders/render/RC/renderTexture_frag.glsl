#version 410 core

out vec4 FragColor;

uniform sampler2D tex;

in vec2 v_Coord;

void main()
{
	FragColor = texture(tex, v_Coord);
}