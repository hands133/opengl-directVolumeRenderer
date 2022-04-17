#version 410 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_TexCoord;

uniform mat4 u_Model;
uniform mat4 u_Transform;
uniform mat4 u_ViewProjection;

void main()
{
	gl_Position = u_ViewProjection * u_Transform * u_Model * vec4(a_Position, 1.0);
}