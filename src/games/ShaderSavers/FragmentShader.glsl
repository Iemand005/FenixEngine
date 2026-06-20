#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

// Ultra-stabiele rotatie matrix
mat2 rot(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}

// De Fractal 'Space-Folding' functie
float map(vec3 p) {
    // Oneindige herhaling van de ruimte (modulo-ruimte)
    p.z = mod(p.z, 8.0) - 4.0;
    
    // Voeg een vloeiende rotatie toe over de diepte-as
    p.xy *= rot(time * 0.1);
    
    // Sla de ruimte herhaaldelijk dubbel (Fractal folding)
    // Dit creëert de complexe geometrische structuren
    for (int i = 0; i < 4; i++) {
        p = abs(p) - vec3(0.6, 0.8, 0.5); // Spiegelen rond assen
        p.xy *= rot(0.5);                 // Draaien
        p.xz *= rot(0.3);                 // Draaien op een andere as
    }
    
    // Genereer scherpe balken/boxen in de gevouwen ruimte
    vec3 d = abs(p) - vec3(0.3, 0.3, 1.5);
    float box = min(max(d.x, max(d.y, d.z)), 0.0) + length(max(d, 0.0));
    
    // Snijd er een cilinder uit voor een 'gang'-effect
    float cilinder = length(p.xy) - 0.15;
    
    return max(box, -cilinder) * 0.8; // Schaalfactor voor raymarching stabiliteit
}

void main() {
    // Normaliseer coördinaten en maak een perfecte aspect ratio aan
    vec2 uv = (gl_FragCoord.xy - 0.5 * resolution.xy) / resolution.y;
    
    // Camera setup - Vlieg oneindig vooruit door de herhaalde ruimte
    vec3 ro = vec3(0.0, 0.0, time * 2.5); 
    vec3 rd = normalize(vec3(uv, 1.1));
    
    // Subtiele camera-schudbeweging voor een dynamisch effect
    rd.xy *= rot(sin(time * 0.5) * 0.1);
    
    // Raymarching
    float dO = 0.0;
    float glow = 0.0;
    int steps = 50;
    float dS = 0.0;
    
    for(int i = 0; i < steps; i++) {
        vec3 p = ro + rd * dO;
        dS = map(p);
        dO += dS;
        
        // Verzamel een intense neon-gloed op basis van hoe dicht de straal langs de fractal scheert
        glow += exp(-dS * 12.0);
        
        if(dO > 30.0 || abs(dS) < 0.001) break;
    }
    
    // Basis belichting en diepte-mist
    float fog = 1.0 / (1.0 + dO * dO * 0.05);
    
    // Genereer een verschuivend cyberpunk kleurenpalet (neon roze/magenta en elektrisch cyaan)
    vec3 colorA = vec3(0.9, 0.0, 0.4); // Neon Magenta
    vec3 colorB = vec3(0.0, 0.8, 1.0); // Neon Cyaan
    
    // Mix de kleuren op basis van de diepte en de tijd
    vec3 glowColor = mix(colorA, colorB, sin(dO * 0.2 + time) * 0.5 + 0.5);
    
    // Voeg de verzamelde gloed toe voor dat intense lichteffect
    vec3 finalColor = glowColor * (glow * 0.035);
    
    // Geef de verre achtergrond een diepe sfeer en voeg mist toe
    finalColor += colorB * (fog * 0.4);
    
    // Cinema-look nabewerking (Vignette, contrast en gamma)
    float vignette = smoothstep(1.2, 0.4, length(uv));
    finalColor *= vignette;
    finalColor = pow(finalColor, vec3(0.4545)); // Gamma 2.2
    finalColor = smoothstep(0.05, 0.95, finalColor); // Contrast boost
    
    FragColor = vec4(finalColor, 1.0);
}
