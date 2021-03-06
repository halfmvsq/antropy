#version 330 core

// Texture coordinates (s, t, p) are in [0.0, 1.0]^3
#define MIN_IMAGE_TEXCOORD vec3( 0.0 )
#define MAX_IMAGE_TEXCOORD vec3( 1.0 )

in VS_OUT // Redeclared vertex shader outputs: now the fragment shader inputs
{
    vec3 ImgTexCoords;
    vec3 SegTexCoords;
    vec2 CheckerCoord;
    vec2 ClipPos;
} fs_in;

layout (location = 0) out vec4 OutColor; // Output RGBA color (pre-multiplied alpha)

uniform sampler3D imgTex; // Texture unit 0: image
uniform usampler3D segTex; // Texture unit 1: segmentation
uniform sampler1D imgCmapTex; // Texture unit 2: image color map (pre-mult RGBA)
uniform sampler1D segLabelCmapTex; // Texutre unit 3: label color map (pre-mult RGBA)

uniform vec2 imgSlopeIntercept; // Slopes and intercepts for image window-leveling
uniform vec2 imgCmapSlopeIntercept; // Slopes and intercepts for the image color maps

uniform vec2 imgThresholds; // Image lower and upper thresholds, mapped to OpenGL texture intensity
uniform float imgOpacity; // Image opacities
uniform float segOpacity; // Segmentation opacities

uniform bool masking; // Whether to mask image based on segmentation

uniform vec2 clipCrosshairs; // Crosshairs in Clip space

// Should comparison be done in x,y directions?
// If x is true, then compare along x; if y is true, then compare along y;
// if both are true, then compare along both.
uniform bvec2 quadrants;

// Should the fixed image be rendered (true) or the moving image (false):
uniform bool showFix;

// Render mode: 0 - normal, 1 - checkerboard, 2 - quadrants, 3 - flashlight
uniform int renderMode;

uniform float aspectRatio;

uniform float flashlightRadius;

// When true, the flashlight overlays the moving image on top of fixed image.
// When false, the flashlight replaces the fixed image with the moving image.
uniform bool flashlightOverlays;

// MIP mode: 0 - none, 1 - max, 2 - mean, 3 - min
uniform int mipMode;

// Half the number of samples for MIP. Set to 0 when mipMode == 0.
uniform int halfNumMipSamples;

// Z view camera direction, represented in texture sampling space
uniform vec3 texSamplingDirZ;


float smoothThreshold( float value, vec2 thresholds )
{
    return smoothstep( thresholds[0] - 0.01, thresholds[0], value ) -
           smoothstep( thresholds[1], thresholds[1] + 0.01, value );
}

float hardThreshold( float value, vec2 thresholds )
{
    return float( thresholds[0] <= value && value <= thresholds[1] );
}

// Check if inside texture coordinates
bool isInsideTexture( vec3 a )
{
    return ( all( greaterThanEqual( a, MIN_IMAGE_TEXCOORD ) ) &&
             all( lessThanEqual( a, MAX_IMAGE_TEXCOORD ) ) );
}


void main()
{
    // Indicator of the quadrant of the crosshairs that the fragment is in:
    bvec2 Q = bvec2( fs_in.ClipPos.x <= clipCrosshairs.x,
                     fs_in.ClipPos.y > clipCrosshairs.y );

    // Distance of the fragment from the crosshairs, accounting for aspect ratio:
    float flashlightDist = sqrt(
        pow( aspectRatio * ( fs_in.ClipPos.x - clipCrosshairs.x ), 2.0 ) +
        pow( fs_in.ClipPos.y - clipCrosshairs.y, 2.0 ) );

    // Flag indicating whether the fragment will be rendered:
    bool doRender = ( 0 == renderMode );

    // If in Checkerboard mode, then render the fragment?
    doRender = doRender || ( ( 1 == renderMode ) &&
        ( showFix == bool( mod( floor( fs_in.CheckerCoord.x ) +
                                floor( fs_in.CheckerCoord.y ), 2.0 ) > 0.5 ) ) );

    // If in Quadrants mode, then render the fragment?
    doRender = doRender || ( ( 2 == renderMode ) &&
        ( showFix == ( ( ! quadrants.x || Q.x ) == ( ! quadrants.y || Q.y ) ) ) );

    // If in Flashlight mode, then render the fragment?
    doRender = doRender || ( ( 3 == renderMode ) &&
        ( ( showFix == ( flashlightDist > flashlightRadius ) ) ||
                       ( flashlightOverlays && showFix ) ) );

    if ( ! doRender ) discard;

    // Image and segmentation masks based on texture coordinates:
    bool imgMask = isInsideTexture( fs_in.ImgTexCoords );
    bool segMask = isInsideTexture( fs_in.SegTexCoords );

    // Look up the image value (after mapping to GL texture units):
    float img = texture( imgTex, fs_in.ImgTexCoords ).r;

    // Number of samples used for computing the final image value:
    int numSamples = 1;

    // Accumulate intensity projection in forwards (+Z) direction:
    for ( int i = 1; i <= halfNumMipSamples; ++i )
    {
        vec3 tc = fs_in.ImgTexCoords + i * texSamplingDirZ;
        if ( ! isInsideTexture( tc ) ) break;

        float a = texture( imgTex, tc ).r;

        img = float( 1 == mipMode ) * max( img, a ) +
              float( 2 == mipMode ) * ( img + a ) +
              float( 3 == mipMode ) * min( img, a );

        ++numSamples;
    }

    // Accumulate intensity projection in backwards (-Z) direction:
    for ( int i = 1; i <= halfNumMipSamples; ++i )
    {
        vec3 tc = fs_in.ImgTexCoords - i * texSamplingDirZ;
        if ( ! isInsideTexture( tc ) ) break;

        float a = texture( imgTex, tc ).r;

        img = float( 1 == mipMode ) * max( img, a ) +
              float( 2 == mipMode ) * ( img + a ) +
              float( 3 == mipMode ) * min( img, a );

        ++numSamples;
    }

    // If using Mean Intensity Projection mode, then normalize by the number of samples:
    img /= mix( 1.0, float( numSamples ), float( 2 == mipMode ) );

    // Look up segmentation texture label value:
    uint seg = texture( segTex, fs_in.SegTexCoords ).r;

    // Apply window/level to normalize image values in [0.0, 1.0] range:
    float imgNorm = clamp( imgSlopeIntercept[0] * img + imgSlopeIntercept[1], 0.0, 1.0 );

    // Compute the image mask:
    float mask = float( imgMask && ( masking && ( seg > 0u ) || ! masking ) );

    // Compute image alpha based on opacity, mask, and thresholds:
    float imgAlpha = imgOpacity * mask * hardThreshold( img, imgThresholds );

    // Compute segmentation alpha based on opacity and mask:
    float segAlpha = segOpacity * float(segMask);

    // Look up image color and apply alpha:
    vec4 imgColor = texture( imgCmapTex, imgCmapSlopeIntercept[0] * imgNorm + imgCmapSlopeIntercept[1] ) * imgAlpha;

    // Look up segmentation color and apply :
    vec4 segColor = texelFetch( segLabelCmapTex, int(seg), 0 ) * segAlpha;

    // Blend colors:
    OutColor = vec4( 0.0, 0.0, 0.0, 0.0 );
    OutColor = imgColor + ( 1.0 - imgColor.a ) * OutColor;
    OutColor = segColor + ( 1.0 - segColor.a ) * OutColor;
}
