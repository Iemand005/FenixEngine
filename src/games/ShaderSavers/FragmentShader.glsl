#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

// Ultra-snelle, hardware-vriendelijke rotatie
mat2 rot(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}

// Een geavanceerde, stabiele 3D feedback-wave-functie
// Deze genereert de complexe, vloeibare 'kwantum-patronen'
vec3 pattern(in vec2 uv) {
    vec2 p = uv * 2.5;
    
    // Versnelde tijd voor brute snelheid
    float t = time * 2.5;
    
    // Feedback-lus 1: Introduceer magnetische stroming
    vec2 q = vec2(
        sin(p.x + p.y + t * 0.5),
        cos(p.x - p.y - t * 0.7)
    );
    
    // Feedback-lus 2: Roteer en vouw de stroming in zichzelf (Warping)
    vec2 r = vec2(
        sin(p.x + q.y + t + sin(p.y * 3.0)),
        cos(p.y + q.x - t + cos(p.x * 3.0))
    );
    r *= rot(t * 0.2);
    
    // Bereken de uiteindelijke interferentie-waarde
    float noiseVal = sin(length(p + r) * 4.0 - t);
    
    // Geef drie verschillende kleurvectoren terug op basis van de feedback-assen
    vec3 col;
    col.r = sin(length(q) * 3.0 + t) * 0.5 + 0.5;
    col.g = cos(length(r) * 4.0 - t * 1.3) * 0.5 + 0.5;
    col.b = sin((q.x + r.y) * 2.0 + t * 0.8) * 0.5 + 0.5;
    
    // Voeg scherpe, vloeibare energielijnen toe (de 'celwanden')
    float randen = smoothstep(0.1, 0.0, abs(noiseVal) - 0.05);
    col += vec3(randen * 0.6);
    
    return col;
}

void main() {
    // Normaliseer coördinaten met een dynamische lens-vervorming (Hypnose-draaikolk)
    vec2 uv = (gl_FragCoord.xy - 0.5 * resolution.xy) / resolution.y;
    
    // Laat de hele ruimte pulseren en tollen over de tijd
    float r = length(uv);
    float angle = atan(uv.y, uv.x);
    
    // Non-Euclidische draaiing: het centrum draait sneller dan de buitenkant
    angle += sin(r * 4.0 - time * 3.0) * 0.8;
    uv = vec2(cos(angle), sin(angle)) * r;
    
    // Zoom continu in en uit via een sinusoïde voor een ademend effect
    uv *= 1.0 + sin(time * 1.5) * 0.3;
    
    // Haal de vloeibare feedback-kleuren op
    vec3 baseColor = pattern(uv);
    
    // --- INTENSE EXTRA FANCY EFFECTEN ---
    
    // 1. Psychedelische Solarisatie (Kleurinversie bij extreme helderheid)
    vec3 trippyColor = abs(baseColor - vec3(sin(time), cos(time * 0.5), sin(time * 1.2)));
    trippyColor = fract(trippyColor * 1.5);
    
    // 2. Chromatische Aberratie-simulatie (RGB-kanalen trekken uit elkaar aan de randen)
    vec3 finalColor;
    finalColor.r = pattern(uv * (1.0 + 0.03 * sin(time))).r;
    finalColor.g = trippyColor.g;
    finalColor.b = pattern(uv * (1.0 - 0.03 * cos(time))).b;
    
    // 3. Ultra-brute contrast versterking (Neon-glow pop)
    finalColor = smoothstep(0.05, 0.95, finalColor);
    finalColor = pow(finalColor, vec3(0.7)); // Maakt kleuren oogverblindend fel
    
    // Vignette die meebeweegt op de kwantum-storm
    float vignette = smoothstep(1.5, 0.3, length(uv) * (1.0 + 0.2 * sin(time * 4.0)));
    finalColor *= vignette;
    
    FragColor = vec4(finalColor, 1.0);
}
