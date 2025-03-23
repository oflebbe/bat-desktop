#version 300 es
precision mediump float;

in vec2 TexCoords;
out vec4 FragColor;

vec3 hslToRgb(float h, float s, float l) {
    float c = (1.0 - abs(2.0 * l - 1.0)) * s;
    float x = c * (1.0 - abs(mod(h / 60.0, 2.0) - 1.0));
    float m = l - c / 2.0;

    vec3 rgb;
    if (h < 60.0) {
        rgb = vec3(c, x, 0.0);
    } else if (h < 120.0) {
        rgb = vec3(x, c, 0.0);
    } else if (h < 180.0) {
        rgb = vec3(0.0, c, x);
    } else if (h < 240.0) {
        rgb = vec3(0.0, x, c);
    } else if (h < 300.0) {
        rgb = vec3(x, 0.0, c);
    } else {
        rgb = vec3(c, 0.0, x);
    }

    return rgb + vec3(m);
}

void main() {
    // Generiere den Hue-Wert basierend auf der Texturkoordinate
    float hue = TexCoords.x * 360.0; // Hue von 0 bis 360

    // Konvertiere HSL zu RGB (Sättigung = 1.0, Helligkeit = 0.5 für volle Farben)
    vec3 rgb = hslToRgb(hue, 1.0, 0.5);

    // Gib die RGB-Farbe aus
    FragColor = vec4(rgb, 1.0);
}
