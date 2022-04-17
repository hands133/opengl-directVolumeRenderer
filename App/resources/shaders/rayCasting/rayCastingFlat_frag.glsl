#version 460 core

in vec2 v_Coord;

out vec4 FragColor;

uniform float v_Min;
uniform float v_Max;

uniform sampler2D t_CoordIn;
uniform sampler2D t_CoordOut;

uniform sampler3D t_Volume;
uniform sampler1D t_TFFunc;

uniform ivec3 t_DIM;

void main()
{
	vec2 screenCoord = v_Coord;
	vec3 cdIn = texture(t_CoordIn, screenCoord).xyz;
	vec3 cdOut = texture(t_CoordOut, screenCoord).xyz;

	float stepSize = 0.002f;

	vec3 dir = cdOut - cdIn;
	if (length(dir) < stepSize)     discard;

	vec3 currentPos = cdIn;
	vec3 color = vec3(0.0, 0.0, 0.0);
	float alpha = 0.0;

	vec3 deltaDir = dir * (stepSize / length(dir));
	float numSamp = length(dir) / stepSize;

	vec3 C = vec3(0.0f);
	float A = 0.0f;

	int i = 0;

	vec3 coord = cdIn;
	while (i++ < numSamp)
	{
		float v = texture(t_Volume, coord).r;
		vec4 tC = texture(t_TFFunc, (v - v_Min) / (v_Max - v_Min));

		tC.a = 1.0f - pow(1.0f - tC.a, stepSize * 32);
		
		C += (1.0f - A) * tC.rgb * tC.a;
		A += (1.0f - A) * tC.a;

		if (A > 1.0f)	break;

		coord = coord + deltaDir;
	}

	A = min(A, 1.0f);
	FragColor = vec4(C, A);
}