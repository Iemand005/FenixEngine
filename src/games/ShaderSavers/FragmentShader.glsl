#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

// Stabiele rotatiematrix voor de vliegbeweging
mat2 rot(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}

// Pure wiskundige regenboog-formule (gebaseerd op positie, geen flitsende color shift)
vec3 getRainbow(float h) {
    vec3 c = cos(vec3(0.0, 2.0, 4.0) + h * 6.28318) * 0.5 + 0.5;
    return c;
}

void main() {
    // Normaliseer coördinaten en pas aspect-ratio toe
    vec2 uv = (gl_FragCoord.xy - 0.5 * resolution.xy) / resolution.y;
    
    // EXTREME VOORUITGAANDE SNELHEID (Lichtsnelheid)
    float speed = time * 14.0; 
    
    // --- BRAND NEW: CAMERA FLYING PATH ---
    // We laten het middelpunt van de tunnel vloeiend wegdraaien.
    // Dit geeft het intense gevoel dat je bochten maakt en dieper de tunnel in duikt.
    vec2 cameraOffset = vec2(
        sin(time * 1.5) * 0.25 + cos(time * 0.7) * 0.1,
        cos(time * 1.2) * 0.20 + sin(time * 0.5) * 0.1
    );
    uv -= cameraOffset;
    
    // Bereken afstand (r) en hoek (angle) vanaf het bewegende camerapunt
    float r = length(uv);
    float angle = atan(uv.y, uv.x);
    
    // Hyper-speed lensvervorming: rekt de hoeken uit voor een waanzinnig snelheidsgevoel
    float fisheye = r * (1.0 + dot(uv, uv) * 0.5);
    
    // Z-as staat voor de diepte in de tunnel (gekoppeld aan de vervormde lens)
    float z = 1.0 / (fisheye + 0.001); 
    
    // Laat de tunnel subtiel om zijn as tollen naarmate je dieper vliegt
    angle += z * 0.08 + time * 0.3;
    
    // --- GEOMETRISCHE CYBER-WANDEN ---
    // X-as: Het aantal panelen/hoeken in de tunnel
    float panelen = 8.0; 
    float gridX = abs(sin(angle * panelen));
    
    // Y-as: De ringen/lijnen die met noodgang over de Z-as naar je toe vliegen
    float gridY = sin(z * 3.5 - speed);
    
    // Vlijmscherpe smoothsteps voor perfecte sub-pixel rendering op 240Hz
    float lijnX = smoothstep(0.07, 0.0, abs(gridX) - 0.008);
    float lijnY = smoothstep(0.12, 0.0, abs(gridY) - 0.008);
    
    // Combineer tot een vlijmscherp, kinetisch raster
    float cyberRaster = max(lijnX * lijnY, lijnX * 0.25);
    
    // --- REGENBOOG MATRIX (STRIKT REGENBOOG, GEEN TIJD-SHIFT) ---
    // De kleuren zitten vast aan de architectuur van de tunnel, wat rust geeft aan de ogen
    float spectrumInrichting = (angle / 6.28318) + (z * 0.03);
    vec3 rainbowColor = getRainbow(spectrumInrichting);
    
    // --- COMPOSITIE & DIEPTE ---
    // Dieptemist zorgt ervoor dat het centrum (de verre toekomst) pikzwart blijft
    float diepteMist = smoothstep(30.0, 1.5, z);
    
    vec3 finalColor = rainbowColor * 0.08 * z;        // Glow in de diepte van de tunnel
    finalColor += rainbowColor * cyberRaster * 3.5;  // Felle neon laserlijnen
    finalColor += vec3(1.0) * pow(cyberRaster, 6.0); // Wit-hete kruispunten die over je scherm spatten
    
    // Pas de vlieg-mist toe
    finalColor *= diepteMist;
    
    // High-refresh monitor optimalisatie (zachte gamma + hard contrast)
    finalColor = smoothstep(0.0, 1.0, finalColor);
    finalColor = pow(finalColor, vec3(0.7)); 
    
    // Dynamische vignette die subtiel reageert op je vliegbeweging
    float edge = smoothstep(1.5, 0.3, length(uv + cameraOffset * 0.5));
    finalColor *= edge;
    
    FragColor = vec4(finalColor, 1.0);
}
