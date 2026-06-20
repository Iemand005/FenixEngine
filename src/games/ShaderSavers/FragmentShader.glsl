#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

float hash(vec2 p)
{
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 34.45);
    return fract(p.x * p.y);
}

void main()
{
    vec2 uv = gl_FragCoord.xy / resolution.xy;

    // center space
    vec2 p = uv * 2.0 - 1.0;
    p.x *= resolution.x / resolution.y;

    float t = time;

    // angle + radius from center
    float r = length(p);
    float a = atan(p.y, p.x);

    // radial "star lanes" (creates streak directions)
    float lanes = sin(a * 40.0);

    // fast outward motion (KEY IDEA)
    float speed = t * 4.5;

    // fake depth layering using repetition
    float z = fract(r * 6.0 - speed);

    // each “layer” has a random star presence
    float id = floor(r * 6.0 - speed);
    float rnd = hash(vec2(id, lanes * 10.0));

    // star mask (sparse)
    float star = step(0.92, rnd);

    // center burst falloff (stars originate from center)
    float burst = smoothstep(1.0, 0.0, r);

    // moving radial streak (gives speed feeling)
    float streak = sin((r * 30.0 - t * 12.0)) * 0.5 + 0.5;

    // star shape (tight points)
    float glow = smoothstep(0.03, 0.0, abs(fract(r * 30.0 - t * 10.0) - 0.5));

    float intensity = star * burst * (0.4 + 0.6 * streak + glow);

    vec3 col = vec3(intensity);

    FragColor = vec4(col, 1.0);
}