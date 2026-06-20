#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

// Razendsnelle rotatiematrix voor de vliegende beweging
mat2 rot(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}

// Afstandsfunctie die de ruimte explodeert in miljoenen neon-fragmenten
float map(vec3 p) {
    // Versnel de tijd drastisch voor brute snelheid
    float speed = time * 3.5;
    
    // Oneindige herhaling op alle assen om een oneindig deeltjesveld te maken
    p.z = mod(p.z + speed, 4.0) - 2.0;
    p.x = mod(p.x + sin(time * 0.5) * 2.0, 4.0) - 2.0;
    p.y = mod(p.y + cos(time * 0.4) * 2.0, 4.0) - 2.0;
    
    // Sla de ruimte wild alle kanten op (Kaleidoscopische chaos)
    p.xy *= rot(p.z * 0.5 + time);
    p.xz *= rot(time * 0.2);
    
    // Creëer flitsende, vlijmscherpe geometrische energielijnen
    vec3 q = abs(p) - vec3(0.3, 0.1, 0.8);
    float box1 = length(max(q, 0.0)) - 0.02;
    
    // Tweede set deeltjes die erdoorheen snijdt
    vec3 q2 = abs(p * 1.5) - vec3(0.5);
    float box2 = length(max(q2, 0.0)) - 0.01;
    
    return min(box1, box2);
}

void main() {
    // Normaliseer coördinaten en voeg een intense hypersnelheid-lensvervorming toe
    vec2 uv = (gl_FragCoord.xy - 0.5 * resolution.xy) / resolution.y;
    
    // Camera-instelling: we kijken recht in de vuurlinie van de deeltjes
    vec3 ro = vec3(0.0, 0.0, 0.0);
    vec3 rd = normalize(vec3(uv, 0.8 - dot(uv, uv) * 0.4)); // Zware fisheye-tunnel-lens
    
    // Laat de hele camera tollen over de tijd
    rd.xy *= rot(time * 0.8);
    rd.xz *= rot(sin(time * 0.4) * 0.3);

    // Snel ladende Raymarching loop gericht op pure gloed (glow)
    float dO = 0.0;
    float glow = 0.0;
    vec3 colorAccum = vec3(0.0);
    
    for(int i = 0; i < 40; i++) {
        vec3 p = ro + rd * dO;
        float dS = map(p);
        dO += max(abs(dS), 0.08); // Grotere stappen voor extreme snelheid en glitches
        
        // Verzamel een krankzinnige hoeveelheid neon-gloed
        float currentGlow = exp(-abs(dS) * 15.0);
        glow += currentGlow;
        
        // Genereer direct een asynchroon, flitsend kleurenpalet per diepteniveau
        vec3 layerColor;
        layerColor.r = sin(dO * 4.0 + time * 5.0) * 0.5 + 0.5;
        layerColor.g = sin(dO * 3.0 - time * 6.0 + 2.0) * 0.5 + 0.5;
        layerColor.b = cos(dO * 5.0 + time * 7.0 + 4.0) * 0.5 + 0.5;
        
        // Voeg de kleur van deze laag met brute kracht toe
        colorAccum += layerColor * currentGlow * 0.4;
        
        if(dO > 15.0) break;
    }
    
    // Voeg een overstuurde centrale lichtflits toe (Hyperdrive effect)
    vec3 finalColor = colorAccum + vec3(glow * 0.05);
    
    // Voeg een regenboog-interferentiepatroon toe in de hoeken van het scherm
    float r = length(uv);
    finalColor.r += sin(r * 20.0 - time * 10.0) * 0.15;
    finalColor.g += sin(r * 25.0 - time * 12.0) * 0.15;
    finalColor.b += cos(r * 18.0 - time * 8.0) * 0.15;
    
    // Brute contrastboost en verzadiging (Geen zachte overgangen, pure lasers)
    finalColor = smoothstep(0.1, 0.9, finalColor);
    finalColor = pow(finalColor, vec3(0.6)); // Maakt de middentonen extreem fel
    
    FragColor = vec4(finalColor, 1.0);
}
