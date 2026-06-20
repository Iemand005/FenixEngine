#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

// 3D Rotatie om assen te vervagen en vloeiende beweging te krijgen
void rotate(inout vec2 p, float a) {
    p = cos(a) * p + sin(a) * vec2(p.y, -p.x);
}

// Pseudo-willekeurige ruis voor de organische plasmastructuur
float noise3D(vec3 p) {
    vec3 s = vec3(7.0, 157.0, 113.0);
    vec3 ip = floor(p);
    vec4 d = vec4(0.0, s.x, s.y, s.x + s.y);
    p -= ip;
    p = p * p * (3.0 - 2.0 * p);
    vec4 h = vec4(dot(ip, s));
    
    // Bereken vloeiende overgangen tussen rasterpunten
    #define noise_mix(h) mix(mix(fract(sin(h) * 43758.5453), fract(sin(h + d.y) * 43758.5453), p.x), mix(fract(sin(h + d.z) * 43758.5453), fract(sin(h + d.w) * 43758.5453), p.x), p.y)
    
    return mix(noise_mix(h), noise_mix(h + d.z * 2.0), p.z);
}

// Fractal Brownian Motion (gelaagde ruis voor complexe details)
float fbm(vec3 p) {
    float v = 0.0;
    float a = 0.5;
    vec3 shift = vec3(100.0);
    for (int i = 0; i < 4; ++i) {
        v += a * noise3D(p);
        p = p * 2.0 + shift;
        a *= 0.5;
    }
    return v;
}

void main() {
    // Normaliseer coördinaten en corrigeer de beeldverhouding
    vec2 uv = (gl_FragCoord.xy - 0.5 * resolution.xy) / resolution.y;
    
    // Camera-asrotatie op basis van de tijd voor een zwevend effect
    vec3 ro = vec3(0.0, 0.0, time * 0.4); 
    vec3 rd = normalize(vec3(uv, 1.0 - dot(uv, uv) * 0.2)); // Subtiele fisheye-lens
    
    // Roteer de blikrichting langzaam over tijd
    rotate(rd.xz, time * 0.05);
    rotate(rd.yz, time * 0.03);

    // Volumetrische raymarching (we 'scannen' de ruimte in lagen)
    float accum = 0.0;
    vec3 colorAccum = vec3(0.0);
    
    for (int i = 0; i < 40; i++) {
        float depth = float(i) * 0.15;
        vec3 pos = ro + rd * depth;
        
        // Transformeer de ruimte voor een dynamisch plasma-effect
        float n = fbm(pos * 1.5 + vec3(0.0, 0.0, -time * 0.2));
        
        // Creëer scherpe energielijnen (cutting planes) binnen de ruis
        float density = smoothstep(0.45, 0.55, n) * smoothstep(0.65, 0.55, n);
        
        // Geef elke dieptelaag een subtiel verschoven kleurenpalet (Nebula-stijl)
        vec3 layerColor = vec3(
            sin(depth * 2.0 + time + 0.0) * 0.5 + 0.5,
            sin(depth * 1.5 + time + 2.0) * 0.5 + 0.5,
            sin(depth * 1.0 + time + 4.0) * 0.5 + 0.5
        );
        
        // Voeg de helderheid van deze laag toe aan het totaal
        accum += density * (1.0 - accum);
        colorAccum += layerColor * density * (1.0 - accum) * 2.5;
        
        if (accum >= 0.95) break; // Stop als het beeld volledig verzadigd is
    }

    // Achtergrondsterren (hoge frequentie fonkelingen)
    float stars = fract(sin(dot(uv, vec2(12.9898, 78.233))) * 43758.5453);
    stars = step(0.997, stars) * (sin(time * 5.0 + uv.x * 10.0) * 0.5 + 0.5);
    
    // Voeg alles samen
    vec3 finalColor = colorAccum + vec3(stars * 0.8);
    
    // Cinema-stijl nabewerking (Vignette & Contrast boost)
    float vignette = 1.0 - dot(uv, uv) * 0.4;
    finalColor *= vignette;
    finalColor = smoothstep(0.0, 1.1, finalColor); 
    
    FragColor = vec4(finalColor, 1.0);
}
