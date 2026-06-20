#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

// Razendsnelle rotatiematrix voor brute kinetische effecten
mat2 rot(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}

void main() {
    // Normaliseer coördinaten en pas perfecte aspect-ratio toe
    vec2 uv = (gl_FragCoord.xy - 0.5 * resolution.xy) / resolution.y;
    
    // EXTREEM OPGEVOERDE SNELHEID voor de beweging
    float t = time * 8.0;
    
    // Bereken afstand en hoek voor de non-euclidische vortex
    float r = length(uv);
    float angle = atan(uv.y, uv.x);
    
    // De draaikolk tolt met mach-snelheid rond
    angle += sin(r * 4.0 - t * 0.5) * 1.8;
    angle -= t * 0.3; // Constante rotatie-versnelling
    
    // Re-projecteer de coördinaten in de snelle vortex
    vec2 p = vec2(cos(angle), sin(angle)) * r;
    
    // Agressieve ruimtelijke feedback-lussen (Domain Warping)
    // Dit perst de stromen samen in vlijmscherpe banen die over het scherm jagen
    p *= 2.5;
    p.x += sin(p.y + t) * 0.7;
    p.y += cos(p.x - t * 1.2) * 0.7;
    p *= rot(t * 0.05); // Subtiele extra rotatie in de kern
    
    p.x -= cos(p.y - t * 0.8) * 0.4;
    p.y -= sin(p.x + t * 1.1) * 0.4;
    
    // Intensiteit van de vloeibare laserstromen
    float stroom = sin(p.x * 2.5 + p.y * 2.5);
    float laser = smoothstep(0.35, 0.0, abs(stroom) - 0.015);
    
    // --- VAST KLEURENPALET (GEEN COLOR SHIFT) ---
    // De kleuren zijn puur gebaseerd op de 2D-positie (p), NIET op de tijd (t)
    vec3 cyaan   = vec3(0.0, 0.9, 1.0);
    vec3 magenta = vec3(1.0, 0.0, 0.6);
    vec3 blauw   = vec3(0.05, 0.1, 0.5);
    
    // Meng de kleuren op basis van de geometrische vorm van de stromen
    vec3 basisKleur = mix(blauw, cyaan, sin(length(p)) * 0.5 + 0.5);
    vec3 neonGlow   = mix(magenta, cyaan, cos(p.x - p.y) * 0.5 + 0.5);
    
    // Combineer de belichting met de hypersnelheid-beweging
    vec3 finalColor = basisKleur * 0.3;          // Diepe vloeibare achtergrond
    finalColor += neonGlow * laser * 2.5;        // Oogverblindende, felle neon-banen
    finalColor += vec3(1.0) * pow(laser, 6.0);   // Wit-hete kernen voor extra pop op 240Hz
    
    // Krachtige omgevingsgloed vanuit het centrum
    finalColor += basisKleur * 0.4 * (1.0 / (r + 0.1));
    
    // Contrastboost voor maximale paneelprestaties
    finalColor = smoothstep(0.0, 1.0, finalColor);
    finalColor = pow(finalColor, vec3(0.7)); // Heldere, levendige middentonen
    
    // Strakke vignette aan de randen
    float edge = smoothstep(1.4, 0.4, r);
    finalColor *= edge;
    
    FragColor = vec4(finalColor, 1.0);
}
