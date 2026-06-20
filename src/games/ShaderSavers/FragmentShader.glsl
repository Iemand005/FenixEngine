#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

// Stabiele rotatiematrix voor cameratotatie
mat2 rot(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}

// Pure wiskundige regenboog-formule (gebaseerd op positie, geen flitsende color shift)
vec3 getRainbow(float h) {
    vec3 c = cos(vec3(0.0, 2.0, 4.0) + h * 6.28318) * 0.5 + 0.5;
    return c;
}

// Anti-aliasing zaagtand/sinus functie om moiré-patronen in de verte te vernietigen
float antiAliasGrid(float wave, float width) {
    // fwidth berekent de verandering per pixel. Dit filtert hoge frequenties in de diepte.
    float w = fwidth(wave); 
    return 1.0 - smoothstep(-width, width, abs(wave) - w);
}

void main() {
    // Normaliseer coördinaten en pas perfecte aspect-ratio toe
    vec2 uv = (gl_FragCoord.xy - 0.5 * resolution.xy) / resolution.y;
    
    // EXTREEM OPGEVOERDE SNELHEID
    float speed = time * 10.0; 
    
    // Bereken de initiële ruwe afstand tot het schermcentrum
    float r_init = length(uv);
    
    // Z-as staat voor de diepte in de tunnel
    float z = 1.0 / (r_init + 0.01);
    
    // --- BRAND NEW: CURVED TUNNEL (DE INNERLIJKE BOCHTEN) ---
    // We buigen de tunnelbuis door het centrum te verschuiven op basis van de diepte (z).
    // Naarmate je dieper kijkt, buigt de tunnel weg via vloeiende sinusgolven.
    vec2 curve;
    curve.x = sin(z * 0.15 + time * 2.0) * 0.35 + cos(z * 0.05 + time) * 0.15;
    curve.y = cos(z * 0.12 + time * 1.5) * 0.25 + sin(z * 0.07 + time) * 0.10;
    
    // Pas de kromming toe op de originele UV-coördinaten
    vec2 curvedUV = uv - curve;
    
    // Bereken de definitieve gecorrigeerde afstand en hoek binnen de gebogen tunnel
    float r = length(curvedUV);
    float angle = atan(curvedUV.y, curvedUV.x);
    
    // Herbereken de diepte op basis van de gebogen afstand
    z = 1.0 / (r + 0.001);
    float tunnelDiepte = z + speed;
    
    // Laat de tunnel heel traag om zijn eigen as tollen tijdens het vliegen
    angle += time * 0.1 + sin(z * 0.02) * 0.2;
    
    // --- GEOMETRISCHE CYBER-WANDEN (MOIRÉ-FREE) ---
    // X-as: 8 strakke lengtelijnen rondom de koker
    float panelen = 8.0; 
    float gridX = sin(angle * panelen);
    
    // Y-as: De dwarsringen die de tunnel in worden gezogen
    float gridY = sin(tunnelDiepte * 1.2);
    
    // Gebruik de fwidth anti-aliasing filter om moiré-interferentie op te lossen.
    // Lijnen in de verte worden automatisch 'geblurret' in plaats van dat ze gaan flikkeren.
    float lijnX = antiAliasGrid(gridX, 0.04);
    float lijnY = antiAliasGrid(gridY, 0.06);
    
    // Combineer de lengtelijnen en dwarsringen tot één solide tunnelraster
    float cyberTunnel = max(lijnX, lijnY * 0.6);
    
    // --- REGENBOOG MATRIX (STRIKT GEKOPPELD AAN GEOMETRIE) ---
    float spectrumInrichting = (angle / 6.28318) + (z * 0.01);
    vec3 rainbowColor = getRainbow(spectrumInrichting);
    
    // --- COMPOSITIE, GLOED & MIST ---
    // Dieptemist zorgt ervoor dat de tunnelbocht in de diepte mooi organisch zwart vervaagt
    float diepteMist = smoothstep(35.0, 1.0, z);
    
    vec3 finalColor = rainbowColor * 0.03 * z;        // Zachte omgevingsgloed in de koker
    finalColor += rainbowColor * cyberTunnel * 4.0;  // Oogverblindende, flikkervrije neon-laserwanden
    finalColor += vec3(1.0) * pow(cyberTunnel, 5.0); // Wit-hete kruispunten voor die 240Hz pop
    
    // Smoor de kleuren in de verre bochten met de mist
    finalColor *= diepteMist;
    
    // High-refresh monitor contrast en gamma optimalisatie
    finalColor = smoothstep(0.0, 1.0, finalColor);
    finalColor = pow(finalColor, vec3(0.75)); 
    
    // Vignette gekoppeld aan de schermranden om het vlieggevoel te kaderen
    float edge = smoothstep(1.5, 0.4, r_init);
    finalColor *= edge;
    
    FragColor = vec4(finalColor, 1.0);
}
