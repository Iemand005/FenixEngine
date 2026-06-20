#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

float hash(float n)
{
    return fract(sin(n) * 43758.5453123);
}

float hash2(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

void main()
{
    vec2 uv = gl_FragCoord.xy / resolution.xy;

    // center space
    vec2 p = uv * 2.0 - 1.0;
    p.x *= resolution.x / resolution.y;

    float t = time;

    vec3 col = vec3(0.0);

    // number of "star lanes" (cheap fake particles)
    // we don't loop; we tile space instead
    vec2 grid = p * 25.0;

    vec2 id = floor(grid);
    vec2 f = fract(grid) - 0.5;

    // per-cell random star existence
    float rnd = hash2(id);

    // only some cells spawn stars
    float starMask = step(0.92, rnd);

    // spawn direction from center (IMPORTANT PART)
    vec2 dir = normalize(id + vec2(hash(id.x), hash(id.y)) - 0.5);

    // velocity (FAST outward motion)
    float speed = 8.0 + hash2(id) * 6.0;

    // position along trajectory
    float dist = t * speed;

    // star moving outward from center
    vec2 starPos = dir * dist * 0.08;

    // relative position to star
    vec2 diff = p - starPos;

    float d = length(diff);

    // stretch trail along motion direction (fake motion blur)
    float trail = 1.0 / (abs(diff.x * dir.x + diff.y * dir.y) * 30.0 + 1.0);

    // star core
    float core = smoothstep(0.08, 0.0, d);

    // combine core + trail
    float intensity = core + trail * 0.6;

    // fade out when too far
    intensity *= smoothstep(3.0, 0.2, length(starPos));

    // apply spawn mask
    intensity *= starMask;

    col += vec3(intensity);

    FragColor = vec4(col, 1.0);
}