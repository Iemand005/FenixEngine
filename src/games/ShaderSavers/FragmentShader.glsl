#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

// Razendsnelle rotatiematrix voor extreme beweging
mat2 rot(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}

void main() {
    // Normaliseer coördinaten en pas perfecte aspect-ratio toe
    vec2 uv = (gl_FragCoord.xy - 0.5 * resolution.xy) / resolution.y;
    
    // Extreem opgevoerde tijdsfactor voor brute snelheid
    float t = time * 5.5;
    
    // Bereken de hoek en afstand tot het centrum voor een hardcore draaikolk-effect
    float r = length(uv);
    float angle = atan(uv.y, uv.x);
    
    // Non-Euclidische vortex: het centrum tolt met mach-snelheid rond
    angle += sin(r * 3.0 - t * 0.8) * 1.5;
    angle -= t * 0.5; // Constante rotatie-versnelling
    
    // Re-projecteer de coördinaten in de draaikolk
    vec2 p = vec2(cos(angle), sin(angle)) * r;
    
    // Transformeer de ruimte via agressieve feedback-lussen (Domain Warping)
    // Dit perst de kleuren samen in vlijmscherpe, vloeiende banen
    p *= 3.0;
    p.x += sin(p.y + t * 1.5) * 0.8;
    p.y += cos(p.x - t * 2.0) * 0.8;
    p *= rot(t * 0.2);
    
    p.x -= cos(p.y - t * 1.2) * 0.5;
    p.y -= sin(p.x + t * 1.8) * 0.5;
    
    // Intensiteit van de vloeibare energiestromen
    float stroom = sin(p.x * 2.0 + p.y * 2.0 + t);
    float laser = smoothstep(0.4, 0.0, abs(stroom) - 0.02);
    
    // --- EXPLOSIEF REGENBOOG PALET (Spectrum Shift) ---
    // Elke pixel krijgt een unieke, constant verschuivende RGB-waarde op basis van snelheid
    vec3 col;
    col.r = sin(length(p) + t * 3.0 + 0.0) * 0.5 + 0.5;
    col.g = sin(length(p) - t * 3.5 + 2.0) * 0.5 + 0.5;
    col.b = cos((p.x - p.y) + t * 4.0 + 4.0) * 0.5 + 0.5;
    
    // Voeg een tweede, contrasterende neon-kleurlaag toe voor maximale intensiteit
    vec3 neonPaars = vec3(0.9, 0.0, 1.0);
    vec3 neonCyaan  = vec3(0.0, 1.0, 0.8);
    vec3 mixKleur = mix(neonPaars, neonCyaan, sin(t + r * 5.0) * 0.5 + 0.5);
    
    // Combineer de overstuurde kleuren met de vloeibare laserstromen
    vec3 finalColor = col * 0.4;                 // Heldere basisvloeistof
    finalColor += mixKleur * laser * 2.5;       // Oogverblindende neon-banen
    finalColor += vec3(1.0) * pow(laser, 8.0);  // Wit-hete kernen in de stromen
    
    // Krachtige omgevingsgloed die vanuit de vortex naar buiten spuit
    finalColor += col * 0.3 * (1.0 / (r + 0.15));
    
    // Brute kleurverzadiging en contrastboost voor die 240Hz pop
    finalColor = smoothstep(0.05, 0.95, finalColor);
    finalColor = pow(finalColor, vec3(0.65)); // Maximale helderheid van de middentonen
    
    // Subtiele vignette om het centrum nóg harder te laten knallen
    float edge = smoothstep(1.5, 0.5, r);
    finalColor *= edge;
    
    FragColor = vec4(finalColor, 1.0);
}
