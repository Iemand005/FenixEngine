#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

float hash(float n)
{
    return fract(sin(n) * 43758.5453123);
}

vec2 hash2(float n)
{
    return vec2(hash(n), hash(n + 17.0)) * 2.0 - 1.0;
}

// distance from point to segment
float lineDist(vec2 p, vec2 a, vec2 b)
{
    vec2 pa = p - a;
    vec2 ba = b - a;

    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}

void main()
{
    vec2 frag = gl_FragCoord.xy;
    vec2 center = resolution * 0.5;

    vec3 col = vec3(0.0);

    float t = time * 3.0; // FAST

    for (int i = 0; i < 200; i++)
    {
        float fi = float(i);

        vec2 dir = normalize(hash2(fi * 12.989));

        float seed = hash(fi * 78.233);

        float speed = mix(0.5, 2.5, hash(fi * 91.7));

        // continuous outward motion (NO fract, NO mod, NO wrap geometry)
        float d  = seed + t * speed;
        float d0 = seed + (t - 0.03) * speed;

        // normalize into visible range smoothly (NO discontinuities)
        d  = d - floor(d);
        d0 = d0 - floor(d0);

        // IMPORTANT: keep motion linear in screen space
        float r  = d  * (resolution.y * 0.6);
        float r0 = d0 * (resolution.y * 0.6);

        vec2 a = center + dir * r0;
        vec2 b = center + dir * r;

        float dist = lineDist(frag, a, b);

        // TRUE 1px line
        float star = step(dist, 0.5);

        float brightness = 0.6 + hash(fi * 9.1) * 0.7;

        col += vec3(star * brightness);
    }

    FragColor = vec4(col, 1.0);
}