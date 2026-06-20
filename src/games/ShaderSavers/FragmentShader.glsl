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

    float t = time * 2.5;

    for (int i = 0; i < 180; i++)
    {
        float fi = float(i);

        // stable random direction
        vec2 dir = normalize(hash2(fi * 12.989));

        float seed = hash(fi * 78.233);

        // speed (keep bounded)
        float speed = mix(0.6, 2.5, hash(fi * 91.7));

        // continuous motion WITHOUT wrap artifacts
        float z  = seed + t * speed;
        float z0 = seed + (t - 0.02) * speed;

        // NO fract → no popping lines
        z  = z - floor(z);
        z0 = z0 - floor(z0);

        // acceleration outward
        float d  = z  * z;
        float d0 = z0 * z0;

        vec2 p1 = center + dir * d  * (resolution.y * 0.6);
        vec2 p0 = center + dir * d0 * (resolution.y * 0.6);

        float dist = lineDist(frag, p0, p1);

        // 1-pixel streak
        float star = step(dist, 0.6);

        float brightness = 0.7 + hash(fi * 9.1) * 0.5;

        col += vec3(star * brightness);
    }

    FragColor = vec4(col, 1.0);
}