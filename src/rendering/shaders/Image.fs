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


float smoothThreshold( float value, vec2 thresholds )
{
    return smoothstep( thresholds[0] - 0.01, thresholds[0], value ) -
           smoothstep( thresholds[1], thresholds[1] + 0.01, value );
}

float hardThreshold( float value, vec2 thresholds )
{
    return float( thresholds[0] <= value && value <= thresholds[1] );
}


void main()
{
    bvec2 Q = bvec2( fs_in.ClipPos.x <= clipCrosshairs.x, fs_in.ClipPos.y > clipCrosshairs.y );

    float flashlightDist = sqrt( pow( aspectRatio * ( fs_in.ClipPos.x - clipCrosshairs.x ), 2.0 ) +
                                 pow( fs_in.ClipPos.y - clipCrosshairs.y, 2.0 ) );

    bool doRender = ( 0 == renderMode );

    doRender = doRender || ( ( 1 == renderMode ) &&
        ( showFix == bool( mod( floor( fs_in.CheckerCoord.x ) + floor( fs_in.CheckerCoord.y ), 2.0 ) > 0.5 ) ) );

    doRender = doRender || ( ( 2 == renderMode ) &&
        ( showFix == ( ( ! quadrants.x || Q.x ) == ( ! quadrants.y || Q.y ) ) ) );

    doRender = doRender || ( ( 3 == renderMode ) &&
        ( showFix == ( flashlightDist > flashlightRadius ) ) );

    if ( ! doRender ) discard;

    // Foreground masks, based on whether texture coordinates are in range [0.0, 1.0]^3:
    bool imgMask = ! ( any( lessThan( fs_in.ImgTexCoords, MIN_IMAGE_TEXCOORD ) ) ||
                       any( greaterThan( fs_in.ImgTexCoords, MAX_IMAGE_TEXCOORD ) ) );

    bool segMask = ! ( any( lessThan( fs_in.SegTexCoords, MIN_IMAGE_TEXCOORD ) ) ||
                       any( greaterThan( fs_in.SegTexCoords, MAX_IMAGE_TEXCOORD ) ) );

    // Look up the image value (after mapping to GL texture units) and label value:
    float img = texture( imgTex, fs_in.ImgTexCoords ).r;
    uint seg = texture( segTex, fs_in.SegTexCoords ).r;

    // Apply window/level and normalize value in [0.0, 1.0]:
    float imgNorm = clamp( imgSlopeIntercept[0] * img + imgSlopeIntercept[1], 0.0, 1.0 );

    // Mask image:
    float mask = float( imgMask && ( masking && ( seg > 0u ) || ! masking ) );

    // Apply opacity, mask, and thresholds for images:
    float alpha = imgOpacity * mask * hardThreshold( img, imgThresholds );

    // Apply colors to the image and segmentation values. Multiply by alpha for images:
    vec4 imgColor = texture( imgCmapTex, imgCmapSlopeIntercept[0] * imgNorm + imgCmapSlopeIntercept[1] ) * alpha;
    vec4 segColor = texelFetch( segLabelCmapTex, int(seg), 0 ) * segOpacity * float(segMask);

    // Blend colors:
    OutColor = vec4( 0.0, 0.0, 0.0, 0.0 );
    OutColor = imgColor + ( 1.0 - imgColor.a ) * OutColor;
    OutColor = segColor + ( 1.0 - segColor.a ) * OutColor;
}
