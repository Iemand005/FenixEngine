#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

void main()
{
    vec2 fragCoord = gl_FragCoord.xy;
    vec2 iResolution = resolution;
    float iTime = time;

    vec2 pos = fragCoord / iResolution;

    pos -= 0.5;
    pos = abs(pos);

    float lines = 40.0;
    float speed = 2.1;
    float shift = iTime * speed;

    if (sqrt(pow(fragCoord.x - (iResolution.x * 0.5), 2.0) +
             pow(fragCoord.y - (iResolution.y * 0.5), 2.0))
        > iResolution.y * 0.5)
    {
        shift *= -1.0;
    }

    float aspectRatio = iResolution.x / iResolution.y;

    vec2 thingie = pos * lines;
    thingie.y /= aspectRatio;

    float col = floor(mod(thingie.y + thingie.x + shift, 1.0) * 2.0);

    FragColor = vec4(vec3(col), 1.0);
}