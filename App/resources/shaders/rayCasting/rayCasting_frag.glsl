#version 410 core

out vec4 FragColor;

uniform float SCR_WIDTH;
uniform float SCR_HEIGHT;

uniform float vMin;
uniform float vMax;
uniform float GAMMA;

uniform sampler2D coordIn;
uniform sampler2D coordOut;

uniform sampler3D volume;
uniform sampler2D tFunc;

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

    vec3 deltaDir = dir * (stepSize / length(dir));
    float numSamp = length(dir) / stepSize;

    float gamma = max(1e-6f, GAMMA);

    for(int i = 0; i < numSamp; ++i)
    {
        float v = texture(volume, currentPos).r;
        vec4 tmpColor = texture(tFunc, vec2((v - vMin) / (vMax - vMin), 0.0f));

<<<<<<< HEAD:App/resources/shaders/rayCasting/rayCasting_frag.glsl
        tmpColor.a = 1.0f - pow(1.0f - tmpColor.a, gamma);
=======
        if(tmpColor.a > 0.0)
        {
            tmpColor.a = 1.0 - pow(1.0 - tmpColor.a, stepSize * 255.0f);
            color += (1.0 - alpha) * tmpColor.rgb * tmpColor.a;
            alpha += (1.0 - alpha) * tmpColor.a;
        }
>>>>>>> dc9fe76782ec893e2599e16da6008652e0f3a337:resources/shaders/rayCasting_frag.glsl

        color += (1.0 - alpha) * tmpColor.rgb * tmpColor.a;
        alpha += (1.0 - alpha) * tmpColor.a;

        currentPos += deltaDir;
    }

    alpha = min(alpha, 1.0f);
	
    FragColor = vec4(color, alpha);
}