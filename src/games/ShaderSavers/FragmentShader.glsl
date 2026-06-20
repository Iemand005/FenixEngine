#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

// Razendsnelle rotatiematrix voor de vectorvelden
mat2 rot(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}

void main() {
    // Normaliseer coördinaten en pas perfecte aspect-ratio toe
    vec2 uv = (gl_FragCoord.xy - 0.5 * resolution.xy) / resolution.y;
    
    // Snelheidsvariabele - geoptimaliseerd voor extreme framerates
    float t = time * 3.0;
    
    // STAP 1: Kinetische lensvervorming (Hypersnelheid Warp)
    float r = length(uv);
    uv *= rot(sin(r * 2.0 - t * 0.5) * 0.4); // Vloeibare torsie per pixel
    
    // STAP 2: Genereren van het non-lineaire Vectorveld
    // We splitsen de ruimte op in sub-grids die onafhankelijk bewegen
    vec2 p1 = uv * 4.0;
    p1.x += sin(p1.y + t) * 0.5;
    p1.y += cos(p1.x - t * 1.5) * 0.5;
    p1 *= rot(t * 0.1);
    
    vec2 p2 = uv * 8.0; // Fijnere laag voor micro-details op 240Hz
    p2.x -= cos(p2.y - t * 2.0) * 0.3;
    p2.y -= sin(p2.x + t * 1.0) * 0.3;
    p2 *= rot(-t * 0.15);
    
    // STAP 3: Venvlochten Moiré & Laser-lijnen
    // Dit triggert de sub-pixel scherpte van je high-refresh panel
    float lijn1 = sin(p1.x + p1.y);
    float lijn2 = cos(p2.x - p2.y);
    
    // Maak de lijnen vlijmscherp met smoothstep (voorkomt aliasing, behoudt vloeiendheid)
    float laser1 = smoothstep(0.03, 0.0, abs(lijn1) - 0.01);
    float laser2 = smoothstep(0.015, 0.0, abs(lijn2) - 0.005);
    
    // STAP 4: Dynamische Hypersnelheid Tunnel-deeltjes
    // Dit geeft de illusie dat er oneindig veel micro-deeltjes voorbijrazen
    float deeltjes = sin(uv.x * 20.0 + t * 4.0) * sin(uv.y * 20.0 - t * 3.0);
    deeltjes = smoothstep(0.92, 0.99, deeltjes * sin(t + r * 10.0));
    
    // STAP 5: High-Refresh Kleurenberekening (Chromatische Doppler-shift)
    // De kleuren verschuiven op basis van de snelheid van de golven
    vec3 col;
    col.r = sin(length(p1) + t * 2.0) * 0.5 + 0.5;
    col.g = sin(length(p2) - t * 3.0 + 2.0) * 0.5 + 0.5;
    col.b = cos((p1.x + p2.y) + t) * 0.5 + 0.5;
    
    // Combineer de laserlijnen en deeltjes met het kleurenveld
    vec3 finalColor = vec3(0.0);
    finalColor += col * laser1 * 1.5;             // Dikke vloeiende neon-stromen
    finalColor += vec3(0.0, 0.8, 1.0) * laser2 * 2.0; // Fijn elektrisch cyaan raster
    finalColor += vec3(1.0, 1.0, 1.0) * deeltjes * 3.0; // Superwitte high-speed flitsen
    
    // Voeg een diepe, pulserende achtergrondgloed toe
    finalColor += col * 0.15 * (1.0 / (r + 0.2));
    
    // STAP 6: Contrast-optimalisatie voor gaming-monitoren
    finalColor = smoothstep(0.0, 1.0, finalColor);
    finalColor = pow(finalColor, vec3(0.8)); // Boost de helderheid van de bewegende delen
    
    // Subtiele cinematische vignette
    float edge = smoothstep(1.4, 0.3, r);
    finalColor *= edge;
    
    FragColor = vec4(finalColor, 1.0);
}
