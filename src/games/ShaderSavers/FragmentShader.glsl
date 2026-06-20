#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

// Ultra-snelle rotatie om de assen te vervormen
mat2 rot(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}

void main() {
    // Normaliseer coördinaten en introduceer een pulserende lens-vervorming
    vec2 uv = (gl_FragCoord.xy - 0.5 * resolution.xy) / resolution.y;
    
    // Stap 1: Breng de ruimte aan het tollen
    uv *= rot(time * 0.05);
    
    // Stap 2: Vloeibare ruimtelijke vervorming (Space Warping)
    // We gebruiken sinusgolven die zichzelf telkens overschrijven
    vec2 p = uv * 3.0;
    for(int i = 1; i < 5; i++) {
        float f = float(i);
        p.x += sin(p.y + time * 0.4 + f) * 0.6 / f;
        p.y += cos(p.x - time * 0.3 + f) * 0.5 / f;
        p *= rot(0.4);
    }
    
    // Stap 3: Creëer een psychedelisch interferentiepatroon (Moiré/Hypnose)
    float trippyPattern = sin(p.x + p.y + time) * 0.5 + 0.5;
    
    // Extra scherpe, hypnotiserende ringen toevoegen die door de chaos heen snijden
    float ringen = sin(length(uv * 8.0) - time * 4.0);
    ringen = step(0.0, ringen); // Maakt harde, rauwe zwart-wit overgangen
    
    // Stap 4: Chromatische Aberratie (Kleurverschuiving)
    // We splitsen R, G en B op basis van verschillende tijdfrequenties voor een 'foute' look
    vec3 col;
    col.r = sin(p.x * 2.0 + time * 1.5) * 0.5 + 0.5;
    col.g = sin(p.y * 2.0 - time * 2.0) * 0.5 + 0.5;
    col.b = cos((p.x + p.y) * 1.5 + time) * 0.5 + 0.5;
    
    // Meng de harde ringen met de vloeibare kleuren
    col = mix(col, vec3(1.0 - col), ringen * 0.3);
    
    // Stap 5: Aggressieve contrast-versterking (Solarisatie effect)
    // Dit haalt het "mooie" verloop weg en maakt het rauw en intens
    col = fract(col * 2.0); // Klapt de kleuren dubbel zodra ze te helder worden
    col = smoothstep(0.1, 0.9, col);
    
    // Subtiele CRT-monitor scanlijn interferentie voor de textuur
    float scanline = sin(gl_FragCoord.y * 1.5) * 0.08;
    col -= scanline;
    
    FragColor = vec4(col, 1.0);
}
