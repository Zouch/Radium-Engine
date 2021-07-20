out vec4 fragColor;

in vec2 varTexcoord;

uniform sampler2D screenTexture;

// from sRGB color space specification
float tosRGB( float c ) {
    if ( c <= 0.0031308 ) { return 12.92 * c; }
    else {
        return 1.055 * pow( c, 1. / 2.4 ) - 0.055;
    }
}
void main() {
    vec2 size  = vec2( textureSize( screenTexture, 0 ) );
    vec4 pixel = vec4( texelFetch( screenTexture, ivec2( varTexcoord.xy * size ), 0 ) );
    // Pixel is linear, transform to sRGB
    // if (pixel.a != 0) {
    pixel.r = tosRGB( pixel.r );
    pixel.g = tosRGB( pixel.g );
    pixel.b = tosRGB( pixel.b );
    //}
    fragColor = pixel;
}
