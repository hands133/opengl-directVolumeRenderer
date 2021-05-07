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
	vec2 coordOnScreen = vec2(gl_FragCoord.x + 0.5, gl_FragCoord.y + 0.5);
	coordOnScreen /= vec2(SCR_WIDTH - 1, SCR_HEIGHT - 1);
	vec3 cdIn = texture(coordIn, coordOnScreen).xyz;
	vec3 cdOut = texture(coordOut, coordOnScreen).xyz;

    float stepSize =  0.001f;

    vec3 dir = cdOut - cdIn;
    if (length(dir) < stepSize) discard;
    
	vec3 currentPos = cdIn;
	vec3 color = vec3(0.0, 0.0, 0.0);
	float alpha = 0.0;
    float disAccu = 0.0;

    vec3 deltaDir = dir * (stepSize / length(dir));
    float numSamp = length(dir) / stepSize;
    
    for(int i = 0; i < numSamp; ++i)
    {
        float v = texture(volume, currentPos).r;
        vec4 tmpColor = texture(tFunc, (v - vMin)/ (vMax - vMin));

        tmpColor.a *= 0.3;
        if(tmpColor.a > 0.0)
        {
            tmpColor.a = 1.0 - pow(1.0 - tmpColor.a, stepSize * 200.0f);
            color += (1.0 - alpha) * tmpColor.rgb * tmpColor.a;
            alpha += (1.0 - alpha) * tmpColor.a;
        }

        currentPos += deltaDir;
        disAccu += stepSize;

        if(disAccu > length(dir))
        {
            if(alpha > 1.0)
            {
                alpha = 1.0;
                break;
            }
            break;
        }
    }

	FragColor = vec4(color, alpha);
}