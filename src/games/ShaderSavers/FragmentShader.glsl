#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

// Super-stabiele rotatiematrix
mat2 rot(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}

// Pure wiskundige regenboog-formule gebaseerd op de positie in de tunnel
vec3 getRainbow(float h) {
    vec3 c = cos(vec3(0.0, 2.0, 4.0) + h * 6.28318) * 0.5 + 0.5;
    return c;
}

// Anti-moiré filter op basis van pixel-afgeleiden
float antiAliasGrid(float wave, float width) {
    float w = fwidth(wave); 
    return 1.0 - smoothstep(-width, width, abs(wave) - w);
}

// --- BRAND NEW: DE STATISCHE, VASTE BOCHT-BUIS ---
// Het verloop van deze buis hangt PUUR af van de 3D-positie p.z, NIET van de tijd!
vec2 getTunnelPath(float z) {
    vec2 path;
    // Gelaagde sinussen over de Z-as creëren een complexe, vaste 3D S-bocht in de ruimte
    path.x = sin(z * 0.18) * 0.65 + cos(z * 0.07) * 0.25;
    path.y = cos(z * 0.15) * 0.45 + sin(z * 0.05) * 0.15;
    return path;
}

// De 3D Afstandsfunctie (SDF) voor de rigide tunnel
float map(vec3 p, out vec2 tunnelUV) {
    // Haal de rotsvaste 3D-kromming van de buis op voor dit diepteniveau (p.z)
    vec2 curve = getTunnelPath(p.z);
    
    // Verschuif de ruimte ten opzichte van de vaste bocht
    vec3 q = p;
    q.xy -= curve;
    
    // Bereken de exacte afstand tot de binnenkant van de 3D-cilinder (straal = 1.2)
    float cilinder = 1.2 - length(q.xy);
    
    // Genereer de textuurcoördinaten op de tunnelwand
    float angle = atan(q.y, q.x);
    tunnelUV = vec2(angle, p.z);
    
    return cilinder;
}

void main() {
    // Normaliseer coördinaten met een perfecte aspect-ratio
    vec2 uv = (gl_FragCoord.xy - 0.5 * resolution.xy) / resolution.y;
    
    // BRUTE VOORUITGAANDE FLY-SPEED (Achtbaansnelheid)
    float speed = time * 9.5; 
    
    // --- 3D CAMERA PATH (VOLGT DE VASTE BOCHTEN) ---
    // De camera bevindt zich in de 3D-ruimte op positie Z = speed.
    // De X- en Y-posities worden berekend met exact hetzelfde pad als de tunnel zelf!
    vec3 ro = vec3(getTunnelPath(speed), speed);
    
    // Bereken waar de camera naartoe kijkt (iets verderop in de vaste bocht)
    float lookAhead = speed + 1.2;
    vec3 target = vec3(getTunnelPath(lookAhead), lookAhead);
    
    // Bouw de 3D-kijkmatrix voor de camera (volgt vloeiend de bocht in)
    vec3 forward = normalize(target - ro);
    vec3 right = normalize(cross(vec3(0.0, 1.0, 0.0), forward));
    vec3 up = cross(forward, right);
    
    // Genereer de 3D-straalrichting (Ray Direction) voor deze pixel
    vec3 rd = normalize(forward * 1.0 + uv.x * right + uv.y * up);
    
    // Laat de camera zachtjes over zijn as tollen in de bochten (g-kracht effect)
    rd.xy *= rot(time * 0.15);
    
    // --- 3D RAYMARCHING LOOP ---
    float dO = 0.0;
    vec2 tunnelUV = vec2(0.0);
    float dS = 0.0;
    int maxSteps = 45;
    
    for(int i = 0; i < maxSteps; i++) {
        vec3 p = ro + rd * dO;
        dS = map(p, tunnelUV);
        dO += dS * 0.85; // Veilig marcheren door de vaste geometrie
        if(dO > 25.0 || abs(dS) < 0.005) break;
    }
    
    // --- GEOMETRISCHE CYBER-WANDEN (Moiré-Vrij) ---
    // X-as: 8 strakke lengtelijnen die de bochten accentueren
    float panelen = 8.0;
    float gridX = sin(tunnelUV.x * panelen);
    
    // Y-as: De dwarsringen die op hun vaste plek in de tunnel blijven liggen
    float gridY = sin(tunnelUV.y * 1.5);
    
    // Anti-aliasing filter toepassen
    float lijnX = antiAliasGrid(gridX, 0.05);
    float lijnY = antiAliasGrid(gridY, 0.07);
    float cyberTunnel = max(lijnX, lijnY * 0.5);
    
    // --- REGENBOOG MATRIX (STRIKT REGENBOOG, GEEN TIJD-SHIFT) ---
    // De kleuren zitten vastgekleefd aan de 3D-architectuur van de buis
    float spectrumInrichting = (tunnelUV.x / 6.28318) + (tunnelUV.y * 0.015);
    vec3 rainbowColor = getRainbow(spectrumInrichting);
    
    // --- COMPOSITIE & 3D DIEPTE ---
    // Dieptemist zorgt ervoor dat de tunnel in de verre, vaste bochten donker wegzakt
    float diepteMist = smoothstep(22.0, 1.5, dO);
    
    vec3 finalColor = rainbowColor * 0.015 * (25.0 - dO); // Interne sfeergloed
    finalColor += rainbowColor * cyberTunnel * 4.0;       // Felle, vaste neon laserlijnen
    finalColor += vec3(1.0) * pow(cyberTunnel, 5.0);      // Wit-hete kruispunten op 240Hz
    
    finalColor *= diepteMist;
    
    // Monitor optimalisatie
    finalColor = smoothstep(0.0, 1.0, finalColor);
    finalColor = pow(finalColor, vec3(0.75)); 
    
    // Fijne, stabiele vignette
    float edge = smoothstep(1.5, 0.4, length(uv));
    finalColor *= edge;
    
    FragColor = vec4(finalColor, 1.0);
}
