#version 400 core

out vec4 FragColor;

in vec3 coord;

uniform float SCR_WIDTH;
uniform float SCR_HEIGHT;

uniform float vMin;
uniform float vMax;

uniform sampler2D coordIn;
uniform sampler2D coordOut;

uniform sampler3D volume;
uniform sampler1D tFunc;

void main()
{
	vec2 coordOnScreen = vec2(gl_FragCoord.x - 0.5, gl_FragCoord.y - 0.5);
	coordOnScreen /= vec2(SCR_WIDTH - 1, SCR_HEIGHT - 1);
	vec3 cdIn = texture(coordIn, coordOnScreen).xyz;
	vec3 cdOut = texture(coordOut, coordOnScreen).xyz;

	vec3 rayL = normalize(cdOut - cdIn);

	vec3 currentPos = cdIn;
	vec3 color = vec3(0.0, 0.0, 0.0);
	float alpha = 0.0;
	float step = 0.0001;

	while (dot(currentPos - cdIn, currentPos - cdOut) <= 0)
	{
		float value = texture(volume, currentPos).r;
		vec4 tfColor = texture(tFunc, (value - vMin) / (vMax - vMin));
		// vec4 tfColor = texture(tFunc, 0.5);

		vec3 colorNow = tfColor.rgb;
		float alphaNow = tfColor.a;

		vec3 colorIn = color;
		float alphaIn = alpha;

		vec3 colorOut = colorIn * alphaIn + colorNow * alphaNow * (1 - alphaIn);
		float alphaOut = alphaIn + alphaNow * (1 - alphaIn);

		color = colorOut;
		alpha = alphaOut;

		if (alpha >= 1.0)
		{
			alpha = 1.0;
			break;
		}

		currentPos += (step * rayL);
	}

	FragColor = vec4(color, alpha);
}