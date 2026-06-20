#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

// Razendsnelle rotatiematrix voor kinetische effecten
mat2 rot(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}

// Dynamische regenboog-formule (gebaseerd op positie, NIET op tijd)
vec3 getRainbow(float h) {
    // Genereert een perfect, vloeiend spectrum (rood, oranje, geel, groen, blauw, paars)
    vec3 c = cos(vec3(0.0, 2.0, 4.0) + h * 6.28318) * 0.5 + 0.5;
    return c;
}

void main() {
    // Normaliseer coördinaten en pas aspect-ratio toe
    vec2 uv = (gl_FragCoord.xy - 0.5 * resolution.xy) / resolution.y;
    
    // EXTREEM OPGEVOERDE SNELHEID voor de wiskunde
    float t = time * 8.0;
    
    // Bereken afstand en hoek voor de non-euclidische vortex
    float r = length(uv);
    float angle = atan(uv.y, uv.x);
    
    // De draaikolk tolt met lichtsnelheid rond
    angle += sin(r * 4.0 - t * 0.4) * 1.8;
    angle -= t * 0.3; 
    
    // Re-projecteer de coördinaten in de snelle vortex
    vec2 p = vec2(cos(angle), sin(angle)) * r;
    
    // Agressieve ruimtelijke feedback-lussen (Domain Warping)
    // Dit perst de banen samen tot vlijmscherpe energiestromen
    p *= 2.5;
    p.x += sin(p.y + t) * 0.7;
    p.y += cos(p.x - t * 1.2) * 0.7;
    p *= rot(t * 0.05); 
    
    p.x -= cos(p.y - t * 0.8) * 0.4;
    p.y -= sin(p.x + t * 1.1) * 0.4;
    
    // Intensiteit van de vloeibare laserstromen
    float stroom = sin(p.x * 2.5 + p.y * 2.5);
    float laser = smoothstep(0.35, 0.0, abs(stroom) - 0.015);
    
    // --- REGENBOOG SPECTRUM (ZONDER COLOR SHIFT OVER TIJD) ---
    // We gebruiken de ruimtelijke positie 'p' en de hoek van de vortex om de regenboog te bepalen
    float spectrumInrichting = atan(p.y, p.x) / 6.28318 + length(p) * 0.1;
    vec3 rainbowColor = getRainbow(spectrumInrichting);
    
    // Combineer de beweging met de regenboog-wiskunde
    vec3 finalColor = rainbowColor * 0.2;        // Zachte, vloeibare regenboog-achtergrond
    finalColor += rainbowColor * laser * 2.5;    // Oogverblindende, felle neon regenboog-banen
    finalColor += vec3(1.0) * pow(laser, 6.0);   // Wit-hete kernen voor maximale pop op 240Hz
    
    // Krachtige omgevingsgloed vanuit het centrum van de vortex
    finalColor += rainbowColor * 0.3 * (1.0 / (r + 0.1));
    
    // Brute contrastboost voor gaming-monitoren
    finalColor = smoothstep(0.0, 1.0, finalColor);
    finalColor = pow(finalColor, vec3(0.7)); 
    
    // Strakke, vloeiende vignette
    float edge = smoothstep(1.4, 0.4, r);
    finalColor *= edge;
    
    FragColor = vec4(finalColor, 1.0);
}
