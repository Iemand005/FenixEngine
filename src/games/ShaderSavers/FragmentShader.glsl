#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

// Super-stabiele rotatiematrix
mat2 rot(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}

// Pure wiskundige regenboog-formule (gebaseerd op positie, geen flitsende color shift)
vec3 getRainbow(float h) {
    vec3 c = cos(vec3(0.0, 2.0, 4.0) + h * 6.28318) * 0.5 + 0.5;
    return c;
}

// Anti-moiré filter op basis van pixel-afgeleiden
float antiAliasGrid(float wave, float width) {
    float w = fwidth(wave); 
    return 1.0 - smoothstep(-width, width, abs(wave) - w);
}

// --- BRAND NEW: DE 3D FLEXIBELE BUIS ---
// Deze functie definieert de daadwerkelijke 3D-vorm van de buigende buis
float map(vec3 p, out vec2 tunnelUV) {
    // 1. Bereken de fysieke 3D-kromming van de buis op basis van de diepte (p.z)
    // De buis slingert fysiek naar links, rechts, boven en beneden in de 3D-ruimte
    vec2 curve;
    curve.x = sin(p.z * 0.2 + time * 1.5) * 0.45 + cos(p.z * 0.1) * 0.15;
    curve.y = cos(p.z * 0.18 + time * 1.2) * 0.35 + sin(p.z * 0.08) * 0.10;
    
    // 2. Pas de 3D-kromming toe op de dwarsdoorsnede van de buis
    vec3 q = p;
    q.xy -= curve;
    
    // 3. Bereken de afstand tot de binnenkant van de 3D-cilinder (radius = 1.3)
    float cilinder = 1.3 - length(q.xy);
    
    // 4. Genereer unieke 3D-textuurcoördinaten voor de wanden van de buis
    // tunnelUV.x = de hoek rondom de buiswand, tunnelUV.y = de diepte over de buiswand
    float angle = atan(q.y, q.x);
    tunnelUV = vec2(angle, p.z);
    
    return cilinder;
}

void main() {
    // Normaliseer coördinaten met perfecte aspect-ratio
    vec2 uv = (gl_FragCoord.xy - 0.5 * resolution.xy) / resolution.y;
    
    // EXTREEM OPGEVOERDE VOORUITGAANDE VLIEGSNELHEID
    float speed = time * 8.0; 
    
    // --- 3D CAMERA SETUP ---
    // De camera bevindt zich in de 3D-ruimte en volgt EXACT dezelfde 3D-kromming
    // Hierdoor vliegt de camera perfect gecentreerd door de flexibele buis heen!
    vec3 ro = vec3(
        sin(speed * 0.2 + time * 1.5) * 0.45 + cos(speed * 0.1) * 0.15,
        cos(speed * 0.18 + time * 1.2) * 0.35 + sin(speed * 0.08) * 0.10,
        speed
    );
    
    // Bereken waar de camera naartoe kijkt (subtiel vooruit in de buis)
    float lookAhead = speed + 1.0;
    vec3 target = vec3(
        sin(lookAhead * 0.2 + time * 1.5) * 0.45 + cos(lookAhead * 0.1) * 0.15,
        cos(lookAhead * 0.18 + time * 1.2) * 0.35 + sin(lookAhead * 0.08) * 0.10,
        lookAhead
    );
    
    // Bouw de 3D-kijkvectoren op
    vec3 forward = normalize(target - ro);
    vec3 right = normalize(cross(vec3(0.0, 1.0, 0.0), forward));
    vec3 up = cross(forward, right);
    
    // Genereer de daadwerkelijke 3D-straalrichting (Ray Direction)
    vec3 rd = normalize(forward * 1.1 + uv.x * right + uv.y * up);
    
    // Laat de camera heel subtiel om zijn as tollen voor dynamiek
    rd.xy *= rot(time * 0.1);
    
    // --- 3D RAYMARCHING LOOP ---
    float dO = 0.0;
    vec2 tunnelUV = vec2(0.0);
    float dS = 0.0;
    int maxSteps = 50;
    
    for(int i = 0; i < maxSteps; i++) {
        vec3 p = ro + rd * dO;
        dS = map(p, tunnelUV);
        dO += dS * 0.9; // Neem veilige stappen binnen de gebogen geometrie
        if(dO > 20.0 || abs(dS) < 0.005) break;
    }
    
    // --- GEOMETRISCHE CYBER-WANDEN (Moiré-Vrij) ---
    float panelen = 8.0;
    float gridX = sin(tunnelUV.x * panelen);
    
    // De ringen bewegen vloeiend mee over de werkelijke 3D-lengte van de buis
    float gridY = sin(tunnelUV.y * 1.5);
    
    float lijnX = antiAliasGrid(gridX, 0.05);
    float lijnY = antiAliasGrid(gridY, 0.07);
    float cyberTunnel = max(lijnX, lijnY * 0.5);
    
    // --- REGENBOOG MATRIX (GEKOPPELD AAN HET 3D OPPERVLAK) ---
    float spectrumInrichting = (tunnelUV.x / 6.28318) + (tunnelUV.y * 0.01);
    vec3 rainbowColor = getRainbow(spectrumInrichting);
    
    // --- COMPOSITIE & 3D DIEPTE ---
    // Dieptemist zorgt ervoor dat de buis in de verre, diepe 3D-bochten pikzwart verdwijnt
    float diepteMist = smoothstep(18.0, 2.0, dO);
    
    vec3 finalColor = rainbowColor * 0.02 * (20.0 - dO); // Zachte interne 3D-gloed
    finalColor += rainbowColor * cyberTunnel * 4.0;      // Felle neon laserwanden
    finalColor += vec3(1.0) * pow(cyberTunnel, 5.0);     // Wit-hete kruispunten voor de 240Hz pop
    
    finalColor *= diepteMist;
    
    // High-refresh monitor contrast en gamma optimalisatie
    finalColor = smoothstep(0.0, 1.0, finalColor);
    finalColor = pow(finalColor, vec3(0.75)); 
    
    // Vignette
    float edge = smoothstep(1.5, 0.4, length(uv));
    finalColor *= edge;
    
    FragColor = vec4(finalColor, 1.0);
}
