#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

// Stabiele rotatiematrix voor de subtiele camera-as beweging
mat2 rot(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}

// Pure wiskundige regenboog-formule gebaseerd op hoeken (geen hardcoded kleuren, geen tijdsverschuiving)
vec3 getRainbow(float h) {
    vec3 c = cos(vec3(0.0, 2.0, 4.0) + h * 6.28318) * 0.5 + 0.5;
    return c;
}

void main() {
    // Normaliseer coördinaten en pas aspect-ratio toe
    vec2 uv = (gl_FragCoord.xy - 0.5 * resolution.xy) / resolution.y;
    
    // EXTREME VOORUITGAANDE SNELHEID
    float speed = time * 12.0; 
    
    // Transformeer 2D-schermcoördinaten naar een 3D-tunnel-perspectief
    float r = length(uv);
    float angle = atan(uv.y, uv.x);
    
    // Z-as staat voor de diepte in de tunnel
    float z = 1.0 / (r + 0.01); 
    
    // Voeg een subtiele, vloeiende torsie toe over de diepte van de tunnel
    angle += z * 0.1;
    
    // Subtiele camerarotatie om het effect dynamisch te houden
    angle += time * 0.2;
    
    // STAP 1: De Geometrische Cyber-Wanden creëren
    // De X-as herhaalt zich rondom de tunnel (het aantal panelen/hoeken)
    float panelen = 8.0; 
    float gridX = abs(sin(angle * panelen));
    
    // De Y-as raast met lichtsnelheid over de diepte-as
    float gridY = sin(z * 4.0 - speed);
    
    // Maak vlijmscherpe laserlijnen van het raster
    float lijnX = smoothstep(0.08, 0.0, abs(gridX) - 0.01);
    float lijnY = smoothstep(0.12, 0.0, abs(gridY) - 0.01);
    
    // Combineer ze tot een strak sci-fi raster
    float cyberRaster = max(lijnX * lijnY, lijnX * 0.3);
    
    // STAP 2: De Regenboog-Projectie (Rainbowy)
    // De regenboog is gekoppeld aan de hoek en diepte van de tunnel wanden
    float spectrumInrichting = (angle / 6.28318) + (z * 0.05);
    vec3 rainbowColor = getRainbow(spectrumInrichting);
    
    // STAP 3: Compositie en Belichting
    // De helderheid neemt natuurlijk af naarmate de tunnel dieper wordt (fog/diepte)
    float diepteMist = smoothstep(25.0, 2.0, z);
    
    vec3 finalColor = rainbowColor * 0.1 * z;         // Zachte interne gloed van de tunnel
    finalColor += rainbowColor * cyberRaster * 3.0;   // Oogverblindende neon-laserlijnen
    finalColor += vec3(1.0) * pow(cyberRaster, 5.0);  // Wit-hete kruispunten voor maximale pop op 240Hz
    
    // Pas de dieptemist toe zodat de tunnel oneindig in het donker verdwijnt
    finalColor *= diepteMist;
    
    // STAP 4: High-Refresh Contrast Optimalisatie
    finalColor = smoothstep(0.0, 1.0, finalColor);
    finalColor = pow(finalColor, vec3(0.75)); // Geeft de kleuren een intens, helder karakter
    
    // Zachte vignette voor een cinematische look
    float edge = smoothstep(1.5, 0.3, r);
    finalColor *= edge;
    
    FragColor = vec4(finalColor, 1.0);
}
