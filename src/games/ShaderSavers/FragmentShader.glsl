#version 330 core

out vec4 FragColor;

uniform vec2 resolution;
uniform float time;

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    vec2 uv = gl_FragCoord.xy / resolution;

    vec2 p = uv * 2.0 - 1.0;
    p.x *= resolution.x / resolution.y;

    float t = time * 0.6;

   vec2 pos = fragCoord / resolution.xy;  // Normalized pixel coordinates (from 0 to 1)
    
    pos -= 0.5; // Shift coordinates from 0 to 1, to -0.5 to 0.5
    pos = abs(pos); // Make coordinates go from 0.5 to 0 and back to 0.5
    
    float lines = 40.0;
    float speed = 2.1;
    float shift = time * speed;
    
    if (sqrt(pow(fragCoord.x - (resolution.x / 2.0), 2.0) + pow(fragCoord.y - (resolution.y / 2.0), 2.0)) > resolution.y / 2.0)
        shift = shift * -1.0;
    
    float aspectRatio = resolution.x / resolution.y;
    
    // Get the thingie multiplied
    vec2 thingie = pos * lines;
    
    // Fix the coordinates to follow aspect ratio again
    thingie.y /= aspectRatio;
     
    // Calculate if the pixel should be black or white
    float col = floor(mod(thingie.y + thingie.x + shift, 1.0) * 2.0);
    
    // Output to screen
    FragColor = vec4(vec3(col),1.0);
}