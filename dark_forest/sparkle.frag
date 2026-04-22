#version 140

// Uniforms   
uniform mat4 Vmatrix;  
uniform sampler2D texSampler; 
uniform float time;

// Inputs from vertex shader
smooth in vec3 position_v;  
smooth in vec2 texCoord_v; 

// Fragment shader output
out vec4 color_f; 

// Texture animation parameters
uniform ivec2 pattern = ivec2(4, 2); // Number of frames in horizontal and vertical direction
uniform float frameDuration = 0.1f;  // Duration of a single animation frame in seconds



// Function to select and sample a specific animation frame from a sprite sheet
vec4 sampleTexture(int frameIndex) {
    // Size of each frame in UV space (inverse of grid dimensions)
    vec2 frameUVSize = vec2(1.0) / vec2(pattern);

    // Calculating which row and column the frame is in
    int column = frameIndex % pattern.x;
    int row    = frameIndex / pattern.x;

    // Computing texture coordinate offset for the frame
    vec2 frameOffset = vec2(column, row) * frameUVSize;

    // Adjusting original texCoord to point within selected frame
    vec2 adjustedTexCoord = texCoord_v * frameUVSize + frameOffset;

    return texture(texSampler, adjustedTexCoord);
}

void main() {
    // Determine which frame to display based on elapsed time
    int currentFrame = int(time / frameDuration);

    // Sample the color from the appropriate frame and assign it to the fragment output
    color_f = sampleTexture(currentFrame);
}