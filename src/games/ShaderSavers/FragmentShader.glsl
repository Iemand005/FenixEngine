#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

// Stabiele rotatiematrix voor de camerarotatie
mat2 rot(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}

// Pure wiskundige regenboog-formule gebaseerd op hoeken (geen hardcoded kleuren, geen flitsende color shift)
vec3 getRainbow(float h) {
    vec3 c = cos(vec3(0.0, 2.0, 4.0) + h * 6.28318) * 0.5 + 0.5;
    return c;
}

void main() {
    // Normaliseer coördinaten en pas perfecte aspect-ratio toe
    vec2 uv = (gl_FragCoord.xy - 0.5 * resolution.xy) / resolution.y;
    
    // EXTREEM OPGEVOERDE VOORUITGAANDE SNELHEID
    float speed = time * 12.0; 
    
    // Subtiele camerarotatie om de vlucht dynamisch te houden
    uv *= rot(time * 0.15);
    
    // Bereken afstand (r) en hoek (angle) vanaf het centrum
    float r = length(uv);
    float angle = atan(uv.y, uv.x);
    
    // Z-as staat voor de diepte in de tunnel.
    // We voegen een kleine epsilon (0.001) toe om deling door nul in het centrum te voorkomen.
    float z = 1.0 / (r + 0.001); 
    
    // --- DE CORRECTE INWAARTSE BEWEGING ---
    // Door 'speed' op te tellen bij de diepte (z), bewegen de ringen en lijnen 
    // gegarandeerd VAN de randen van het scherm NAAR het diepe centrum toe.
    float tunnelDiepte = z + speed;
    
    // STAP 1: De Geometrische Cyber-Wanden construeren
    // De X-as herhaalt zich rondom de tunnel (8 strakke, doorlopende lengtelijnen)
    float panelen = 8.0; 
    float gridX = sin(angle * panelen);
    
    // De Y-as vormt de dwarsringen die de tunnel in worden gezogen
    float gridY = sin(tunnelDiepte * 1.5);
    
    // Maak vlijmscherpe, doorlopende laserlijnen in plaats van losse stipjes of dots
    // We schalen de dikte met 'r' zodat de lijnen in de diepte mooi dun en scherp blijven
    float lijnX = smoothstep(0.05 * r, 0.0, abs(gridX) - 0.005);
    float lijnY = smoothstep(0.08 * r, 0.0, abs(gridY) - 0.005);
    
    // Combineer de lengtelijnen en dwarsringen tot één solide tunnelraster
    float cyberTunnel = max(lijnX, lijnY * 0.5);
    
    // STAP 2: De Regenboog-Projectie (Rainbowy)
    // De regenboog zit vastgelijmd aan de hoek van de tunnelwanden (geen color shift over tijd)
    float spectrumInrichting = (angle / 6.28318) + (z * 0.02);
    vec3 rainbowColor = getRainbow(spectrumInrichting);
    
    // STAP 3: Compositie en Belichting
    // Dieptemist zorgt ervoor dat het verre centrum (het verdwijnpunt) mooi donker blijft
    float diepteMist = smoothstep(40.0, 2.0, z);
    
    vec3 finalColor = rainbowColor * 0.05 * z;        // Zachte interne gloed diep in de koker
    finalColor += rainbowColor * cyberTunnel * 3.5;  // Oogverblindende, doorlopende neon-laserwanden
    finalColor += vec3(1.0) * pow(cyberTunnel, 6.0); // Wit-hete kruispunten voor maximale pop op 240Hz
    
    // Breng de mist aan zodat je echt de diepte in kijkt
    finalColor *= diepteMist;
    
    // STAP 4: High-Refresh Monitor Optimalisatie
    finalColor = smoothstep(0.0, 1.0, finalColor);
    finalColor = pow(finalColor, vec3(0.75)); // Geeft de kleuren een intens, helder karakter
    
    // Fijne vignette voor de ultieme focus op de tunnel
    float edge = smoothstep(1.4, 0.3, r);
    finalColor *= edge;
    
    FragColor = vec4(finalColor, 1.0);
}
