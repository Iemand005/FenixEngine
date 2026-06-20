#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

// Ultra-stabiele 2D rotatiematrix
mat2 rot(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}

// Afstandsfunctie (SDF) voor elegante, golvende 3D-linten
float map(vec3 p) {
    // Roteer de ruimte heel traag voor een statig effect
    p.xy *= rot(time * 0.05);
    p.xz *= rot(time * 0.03);
    
    // Creëer golvingen in de 3D-ruimte (alsof het zijden doeken zijn)
    float wave1 = sin(p.x * 0.8 + time * 0.4) * cos(p.z * 0.6 + time * 0.2) * 0.4;
    float wave2 = cos(p.y * 0.7 - time * 0.3) * sin(p.z * 0.5 + time * 0.1) * 0.3;
    
    // Bereken de afstand tot een oneindig vloeiend 3D-oppervlak
    float laken = abs(p.y + wave1 + wave2) - 0.08;
    
    // Voeg een tweede, gekruiste structuur toe voor diepte-gelaagdheid
    vec3 p2 = p;
    p2.xy *= rot(1.0); // Kruislingse hoek
    float wave3 = sin(p2.x * 0.9 - time * 0.2) * 0.5;
    float laken2 = abs(p2.z + wave3) - 0.05;
    
    // Combineer beide linten met een zachte overgang
    float k = 0.5;
    float h = clamp(0.5 + 0.5 * (laken2 - laken) / k, 0.0, 1.0);
    return mix(laken2, laken, h) - k * h * (1.0 - h);
}

// Bereken de 3D-normalen (oppervlakterichting) voor high-end belichting
vec3 getNormal(vec3 p) {
    vec2 e = vec2(0.001, 0.0);
    float d = map(p);
    vec3 n = d - vec3(map(p - e.xyy), map(p - e.yxy), map(p - e.yyx));
    return normalize(n);
}

void main() {
    // Normaliseer coördinaten met perfecte aspect-ratio correctie
    vec2 uv = (gl_FragCoord.xy - 0.5 * resolution.xy) / resolution.y;
    
    // Camera setup (subtiel ademend effect via de Z-as)
    vec3 ro = vec3(0.0, 0.0, -4.5 + sin(time * 0.2) * 0.2);
    vec3 rd = normalize(vec3(uv, 1.5)); // Smalle lens voor een cinematische look
    
    // Raymarching loop
    float dO = 0.0;
    int maxSteps = 64;
    float dS = 0.0;
    
    for(int i = 0; i < maxSteps; i++) {
        vec3 p = ro + rd * dO;
        dS = map(p);
        dO += dS;
        if(dO > 15.0 || abs(dS) < 0.001) break;
    }
    
    // Basis achtergrond: een minimalistisch, studio-achtig verloop (soft vignette)
    vec3 backgroundColor = vec3(0.03, 0.03, 0.04) + vec3(0.04, 0.04, 0.06) * (1.0 - length(uv));
    vec3 finalColor = backgroundColor;
    
    if(dS < 0.001) {
        // We hebben de 3D-sculptuur geraakt!
        vec3 p = ro + rd * dO;
        vec3 n = getNormal(p);
        
        // Studio-belichting opzetten (Luxe 'Rim Lighting' op de randen)
        vec3 lightPos1 = vec3(3.0, 5.0, -3.0);  // Warme key-light
        vec3 lightPos2 = vec3(-4.0, -2.0, -2.0); // Koele fill-light
        
        vec3 l1 = normalize(lightPos1 - p);
        vec3 l2 = normalize(lightPos2 - p);
        
        // Diffuse belichting
        float diff1 = max(dot(n, l1), 0.0);
        float diff2 = max(dot(n, l2), 0.0);
        
        // Specular (glanzende highlights alsof het gepolijst glas/metaal is)
        vec3 ref = reflect(rd, n);
        float spec1 = pow(max(dot(ref, l1), 0.0), 32.0);
        float spec2 = pow(max(dot(ref, l2), 0.0), 16.0);
        
        // Luxe Kleurenpalet definiëren
        vec3 obsidian = vec3(0.08, 0.08, 0.1);       // Diep basismateriaal
        vec3 champagneGold = vec3(0.95, 0.75, 0.45);  // Warme highlights
        vec3 platinum = vec3(0.8, 0.85, 0.9);         // Subtiele koele gloed
        
        // Combineer de schaduwen en lichten
        vec3 matteColor = mix(obsidian, platinum * 0.3, diff2);
        vec3 glossColor = champagneGold * spec1 * 0.8 + platinum * spec2 * 0.4;
        
        finalColor = matteColor + glossColor;
        
        // Ambient Occlusion (voeg diepe schaduwen toe in de plooien)
        float ao = clamp(1.0 - (dO * 0.08), 0.0, 1.0);
        finalColor *= ao;
    }
    
    // Subtiele volumetrische diepte-mist om de achtergrond te blenden
    float fog = smoothstep(3.0, 10.0, dO);
    finalColor = mix(finalColor, backgroundColor, fog * 0.6);
    
    // High-end nabewerking: Gamma 2.2 en filmisch contrast
    finalColor = pow(finalColor, vec3(0.4545)); 
    finalColor = smoothstep(-0.02, 1.02, finalColor);
    
    FragColor = vec4(finalColor, 1.0);
}
