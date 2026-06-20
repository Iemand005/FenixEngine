#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

// Vloeiende rotatiematrix voor de organische golven
mat2 rot(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}

void main() {
    // Normaliseer coördinaten met een perfecte aspect-ratio
    vec2 uv = (gl_FragCoord.xy - 0.5 * resolution.xy) / resolution.y;
    
    // Rustige, constante tijdsfactor voor vloeiende animatie
    float t = time * 0.4;
    
    // Bereken afstand tot het centrum voor zachte lens-effecten
    float r = length(uv);
    
    // Subtiele, vloeibare torsie die meebeweegt met de diepte
    uv *= rot(sin(r * 1.5 - t) * 0.2);
    
    // Gelaagde golfberekening (Domain Warping) voor een zijde-achtige textuur
    // Geen harde deeltjes, pure vloeiende stromen
    vec2 p1 = uv * 2.0;
    p1.x += sin(p1.y + t) * 0.7;
    p1.y += cos(p1.x - t * 0.8) * 0.6;
    p1 *= rot(t * 0.05);
    
    vec2 p2 = uv * 4.0;
    p2.x -= cos(p2.y - t * 1.2) * 0.4;
    p2.y -= sin(p2.x + t * 0.6) * 0.4;
    p2 *= rot(-t * 0.08);
    
    // Creëer zachte, dromerige interferentielijnen (alsof het rook of zijde is)
    float stroom1 = sin(p1.x + p1.y + t);
    float stroom2 = cos(p2.x - p2.y - t);
    
    // Gebruik brede smoothsteps voor een fluweelzachte gloed in plaats van scherpe lasers
    float glow1 = smoothstep(0.6, 0.0, abs(stroom1));
    float glow2 = smoothstep(0.4, 0.0, abs(stroom2));
    
    // Een elegant, rustgevend kleurenpalet (Kosmisch diepblauw, violet en zacht cyaan)
    vec3 diepBlauw  = vec3(0.02, 0.05, 0.15);
    vec3 pastelCyaan = vec3(0.3, 0.75, 0.9);
    vec3 zijdePaars  = vec3(0.55, 0.3, 0.85);
    
    // Meng de kleuren vloeiend op basis van de bewegende vectorvelden
    vec3 mixKleur1 = mix(diepBlauw, zijdePaars, sin(length(p1) + t) * 0.5 + 0.5);
    vec3 mixKleur2 = mix(mixKleur1, pastelCyaan, cos(length(p2) - t) * 0.5 + 0.5);
    
    // Combineer de zachte gloed-lagen voor een gelaagd Aurora-effect
    vec3 finalColor = diepBlauw;
    finalColor += mixKleur2 * glow1 * 0.6;
    finalColor += pastelCyaan * glow2 * 0.3;
    
    // Voeg een heel subtiele, zachte omgevingsgloed toe vanuit het centrum
    finalColor += mixKleur1 * 0.12 * (1.0 / (r + 0.4));
    
    // High-end, filmische nabewerking (geen harde contrast-pops)
    finalColor = pow(finalColor, vec3(0.4545)); // Gamma 2.2 voor perfecte kleurovergangen
    finalColor = smoothstep(-0.05, 1.05, finalColor); // Zachte afronding van zwart- en witwaarden
    
    // Elegante, vloeiende vignette aan de randen van het scherm
    float edge = smoothstep(1.3, 0.4, r);
    finalColor *= edge;
    
    FragColor = vec4(finalColor, 1.0);
}
