#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

// Veilige rotatie-functie
mat2 rot(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}

// Gladde minimum-functie voor het organisch samensmelten van vormen (metaballs effect)
float smin(float a, float b, float k) {
    float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
    return mix(b, a, h) - k * h * (1.0 - h);
}

// De 3D-wereld definitie
float map(vec3 p) {
    // Roteer de hele ruimte langzaam voor dynamiek
    p.xy *= rot(time * 0.1);
    p.xz *= rot(time * 0.05);
    
    // Vorm 1: Een pulserende centrale bol
    float sphere1 = length(p) - (1.2 + 0.3 * sin(time * 2.0));
    
    // Vorm 2: Satelliet-bollen die erdoorheen zweven
    vec3 p2 = p;
    p2.x += sin(time * 1.5) * 2.0;
    p2.z += cos(time * 1.8) * 1.5;
    float sphere2 = length(p2) - 0.6;
    
    // Vorm 3: Een vervormde ring (torus) rondom het centrum
    vec3 p3 = p;
    p3.yz *= rot(time * 0.8);
    vec2 q = vec2(length(p3.xz) - 2.5, p3.y);
    float torus = length(q) - 0.2;
    
    // Smelt de vormen vloeiend samen tot één organische vloeistof
    float vloeistof = smin(sphere1, sphere2, 0.6);
    vloeistof = smin(vloeistof, torus, 0.4);
    
    // Voeg een subtiele rimpeling toe aan het oppervlak
    vloeistof += sin(p.x * 4.0 + time) * sin(p.y * 4.0 + time) * sin(p.z * 4.0) * 0.05;
    
    return vloeistof;
}

void main() {
    // Normaliseer coördinaten
    vec2 uv = (gl_FragCoord.xy - 0.5 * resolution.xy) / resolution.y;
    
    // Camera setup
    vec3 ro = vec3(0.0, 0.0, -5.0); // Camera positie (achteruit geplaatst)
    vec3 rd = normalize(vec3(uv, 1.0)); // Kijkrichting
    
    // Raymarching loop (zonder ingewikkelde diepte-berekeningen)
    float dO = 0.0;
    int maxSteps = 60;
    float dS = 0.0;
    
    for(int i = 0; i < maxSteps; i++) {
        vec3 p = ro + rd * dO;
        dS = map(p);
        dO += dS;
        if(dO > 10.0 || abs(dS) < 0.001) break;
    }
    
    // Kleurenberekening op basis van hoe dicht de straal bij het object kwam
    vec3 color = vec3(0.0);
    
    if(dS < 0.001) {
        // We hebben het object geraakt! Bereken een psychedelische gloed
        vec3 p = ro + rd * dO;
        
        // Kleur op basis van de 3D-coördinaten en tijd
        color.r = 0.5 + 0.5 * sin(p.x + time);
        color.g = 0.5 + 0.5 * sin(p.y + time * 1.3 + 2.0);
        color.b = 0.5 + 0.5 * sin(p.z + time * 1.6 + 4.0);
        
        // Voeg schaduw/diepte toe op basis van het aantal stappen (ambient occlusion)
        color *= 1.2 - (dO * 0.15);
    } else {
        // Achtergrond: een diepe kosmische verloopkleur (gradient)
        color = vec3(0.02, 0.01, 0.05) * (1.0 - length(uv));
    }
    
    // Voeg een dromerige gloed (neon aura) toe rondom de randen
    color += vec3(0.3, 0.1, 0.5) * (1.0 / (1.0 + dO * dO * 0.2));
    
    // Vloeiende overgang en gamma correctie
    color = clamp(color, 0.0, 1.0);
    color = pow(color, vec3(0.4545));
    
    FragColor = vec4(color, 1.0);
}
