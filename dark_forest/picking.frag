// picking.frag
#version 140
uniform vec3 pickingColor; // unique color per object
out vec4 fragColor;
void main() {
    fragColor = vec4(pickingColor, 1.0);
}
