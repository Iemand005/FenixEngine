#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

// Rotatiematrix voor dynamische beweging
mat2 rot(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}

// Gyroid-patroon voor organische, complexe structuren
float gyroid(vec3 p) {
    p.xy *= rot(time * 0.1);
    p.xz *= rot(time * 0.07);
    return dot(sin(p), cos(p.zxy));
}

// Map-functie voor raymarching (bepaalt de vorm van de tunnel)
float map(vec3 p) {
    vec3 q = p;
    q.xy *= rot(p.z * 0.15 + time * 0.05); // Torsie in de tunnel
    
    // Oneindige herhaling in de ruimte voor een tunneleffect
    float tunnel = length(q.xy) - 2.0; 
    
    // Voeg detail toe met de gyroid-ruis
    float noise = gyroid(p * 2.0) * 0.35;
    
    return max(-tunnel, noise);
}

void main() {
    // Normaliseer coördinaten (aspect ratio correctie)
    vec2 uv = (gl_FragCoord.xy - 0.5 * resolution.xy) / resolution.y;
    
    // Camera-instellingen
    vec3 ro = vec3(0.0, 0.0, time * 0.8); // Camera beweegt vooruit
    vec3 rd = normalize(vec3(uv, 1.2));   // Ray direction
    
    // Raymarching loop
    float dO = 0.0;
    float glow = 0.0;
    for(int i = 0; i < 80; i++) {
        vec3 p = ro + rd * dO;
        float dS = map(p);
        dO += dS * 0.5; // Kleinere stappen voor vloeiendere details
        
        // Verzamel 'glow' op basis van hoe dicht de straal bij objecten komt
        glow += exp(-dS * 8.0); 
        
        if(dO > 20.0 || dS < 0.001) break;
    }
    
    // Mist en diepteberekening
    float fog = 1.0 / (1.0 + dO * dO * 0.1);
    
    // Dynamische kleurengeneratie (kosmisch paars, neonblauw en goud)
    vec3 baseCol = vec3(0.1, 0.02, 0.2);
    vec3 glowCol = vec3(0.5 + 0.5 * sin(time + dO), 0.2, 0.8 + 0.2 * cos(time));
    vec3 finalCol = baseCol * glow;
    
    // Voeg de intense gloed en chromatische aberratie-achtige verschuiving toe
    finalCol += glowCol * (glow * 0.04);
    finalCol += vec3(0.2, 0.6, 1.0) * fog * 2.0;
    
    // Contrast en gamma-correctie
    finalCol = pow(finalCol, vec3(0.4545)); // Gamma 2.2
    finalCol = smoothstep(0.0, 1.0, finalCol);
    
    FragColor = vec4(finalCol, 1.0);
}
