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

    float t = time * 0.1;

    for (int i = 0; i < 180; i++)
    {
        float fi = float(i);

        vec2 dir = normalize(hash2(fi * 12.989));

        float seed = hash(fi * 78.233);

        float speed = mix(1.5, 4.0, hash(fi * 91.7));

        // CLEAN lifetime (0..1), no spatial wrapping
        float life  = fract(seed + t * speed);
        float life0 = fract(seed + (t - 0.02) * speed);

        // IMPORTANT: kill wrap discontinuity
        float fade  = smoothstep(0.0, 0.1, life) * smoothstep(1.0, 0.9, life);
        float fade0 = smoothstep(0.0, 0.1, life0) * smoothstep(1.0, 0.9, life0);

        // outward motion (NO mod, NO wrapping in geometry)
        float d  = life  * life;
        float d0 = life0 * life0;

        vec2 a = center + dir * d0 * (resolution.y * 0.6);
        vec2 b = center + dir * d  * (resolution.y * 0.6);

        float dist = lineDist(frag, a, b);

        float star = step(dist, 0.5);

        float brightness = 0.6 + hash(fi * 9.1) * 0.7;

        col += vec3(star * brightness * fade * fade0);
    }

    FragColor = vec4(col, 1.0);
}