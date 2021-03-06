#version 330 core

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

uniform vec2 imgSlopeIntercept; // Slopes and intercepts for image normalization and window-leveling
uniform vec2 imgSlopeInterceptLargest; // Slopes and intercepts for image normalization
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

// Edge properties:
uniform bool thresholdEdges; // Threshold the edges
uniform float edgeMagnitude; // Magnitude of edges to compute
uniform bool overlayEdges; // Overlay edges on image
uniform bool colormapEdges; // Apply colormap to edges
uniform vec4 edgeColor; // RGBA, pre-multiplied by alpha
//uniform bool useFreiChen;

uniform vec3 texSamplingDirX;
uniform vec3 texSamplingDirY;


// Sobel edge detection convolution filters:
// Scharr version (see https://en.wikipedia.org/wiki/Sobel_operator)
const float A = 1.0;
const float B = 2.0;

const mat3 Filter_Sobel[2] = mat3[](
    mat3( A, B, A, 0.0, 0.0, 0.0, -A, -B, -A ),
    mat3( A, 0.0, -A, B, 0.0, -B, A, 0.0, -A )
);

const float SobelFactor = 1.0 / ( 2.0*A + B );

// Frei-Chen edge detection convolution filters:
//const mat3 Filter_FC[9] = mat3[](
//    1.0 / ( 2.0 * sqrt(2.0) ) * mat3( 1.0, sqrt(2.0), 1.0, 0.0, 0.0, 0.0, -1.0, -sqrt(2.0), -1.0 ),
//    1.0 / ( 2.0 * sqrt(2.0) ) * mat3( 1.0, 0.0, -1.0, sqrt(2.0), 0.0, -sqrt(2.0), 1.0, 0.0, -1.0 ),
//    1.0 / ( 2.0 * sqrt(2.0) ) * mat3( 0.0, -1.0, sqrt(2.0), 1.0, 0.0, -1.0, -sqrt(2.0), 1.0, 0.0 ),
//    1.0 / ( 2.0 * sqrt(2.0) ) * mat3( sqrt(2.0), -1.0, 0.0, -1.0, 0.0, 1.0, 0.0, 1.0, -sqrt(2.0) ),
//    1.0 / 2.0 * mat3( 0.0, 1.0, 0.0, -1.0, 0.0, -1.0, 0.0, 1.0, 0.0 ),
//    1.0 / 2.0 * mat3( -1.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, -1.0 ),
//    1.0 / 6.0 * mat3( 1.0, -2.0, 1.0, -2.0, 4.0, -2.0, 1.0, -2.0, 1.0 ),
//    1.0 / 6.0 * mat3( -2.0, 1.0, -2.0, 1.0, 4.0, 1.0, -2.0, 1.0, -2.0 ),
//    1.0 / 3.0 * mat3( 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 )
//);


float smoothThreshold( float value, vec2 thresholds )
{
    return smoothstep( thresholds[0] - 0.01, thresholds[0], value ) -
           smoothstep( thresholds[1], thresholds[1] + 0.01, value );
}

float hardThreshold( float value, vec2 thresholds )
{
    return float( thresholds[0] <= value && value <= thresholds[1] );
}

bool isInsideTexture( vec3 a )
{
    return ( all( greaterThanEqual( a, MIN_IMAGE_TEXCOORD ) ) &&
             all( lessThanEqual( a, MAX_IMAGE_TEXCOORD ) ) );
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
        ( ( showFix == ( flashlightDist > flashlightRadius ) ) || ( flashlightOverlays && showFix ) ) );

    if ( ! doRender ) discard;

    // Foreground masks, based on whether texture coordinates are in range [0.0, 1.0]^3:
    bool imgMask = isInsideTexture( fs_in.ImgTexCoords );
    bool segMask = isInsideTexture( fs_in.SegTexCoords );

    // Image values in a 3x3 neighborhood:
    mat3 V;

    for ( int j = 0; j <= 2; ++j )
    {
        for ( int i = 0; i <= 2; ++i )
        {
            vec3 texSamplingPos = float(i - 1) * texSamplingDirX + float(j - 1) * texSamplingDirY;
            float v = texture( imgTex, fs_in.ImgTexCoords + texSamplingPos ).r;

            // Apply maximum window/level to normalize value in [0.0, 1.0]:
            V[i][j] = imgSlopeInterceptLargest[0] * v + imgSlopeInterceptLargest[1];
        }
    }

    // Convolutions for all masks:
    float C_Sobel[2];
    for ( int i = 0; i <= 1; ++i )
    {
        C_Sobel[i] = dot( Filter_Sobel[i][0], V[0] ) + dot( Filter_Sobel[i][1], V[1] ) + dot( Filter_Sobel[i][2], V[2] );
        C_Sobel[i] *= C_Sobel[i];
    }

    // Gradient magnitude:
    float gradMag_Sobel = SobelFactor * sqrt( C_Sobel[0] + C_Sobel[1] ) / max( edgeMagnitude, 0.01 );

//    float C_FC[9];
//    for ( int i = 0; i <= 8; ++i )
//    {
//        C_FC[i] = dot( Filter_FC[i][0], V[0] ) + dot( Filter_FC[i][1], V[1] ) + dot( Filter_FC[i][2], V[2] );
//        C_FC[i] *= C_FC[i];
//    }

//    float M_FC = ( C_FC[0] + C_FC[1] ) + ( C_FC[2] + C_FC[3] );
//    float S_FC = ( C_FC[4] + C_FC[5] ) + ( C_FC[6] + C_FC[7] ) + ( C_FC[8] + M_FC );
//    float gradMag_FC = sqrt( M_FC / S_FC ) / max( edgeMagnitude, 0.01 );

    // Choose Sobel or Frei-Chen:
//    float gradMag = mix( gradMag_Sobel, gradMag_FC, float(useFreiChen) );
    float gradMag = gradMag_Sobel;

    // If thresholdEdges is true, then threshold gradMag against edgeMagnitude:
    gradMag = mix( gradMag, float( gradMag > edgeMagnitude ), float(thresholdEdges) );

    // Get the image and segmentation label values:
    float img = texture( imgTex, fs_in.ImgTexCoords ).r;
    uint seg = texture( segTex, fs_in.SegTexCoords ).r;

    // Apply window/level and normalize value in [0.0, 1.0]:
    float imgNorm = clamp( imgSlopeIntercept[0] * img + imgSlopeIntercept[1], 0.0, 1.0 );

    // Mask that accounts for the image boundaries and (if masking is true) the segmentation mask:
    float mask = float( imgMask && ( masking && ( seg > 0u ) || ! masking ) );

    // Alpha accounts for opacity, masking, and thresholding:
    // Alpha will be applied to the image and edge layers.
    float alpha = imgOpacity * mask * hardThreshold( img, imgThresholds );

    // Apply color map to the image intensity:
    // Disable the image color if overlayEdges is false.
    vec4 imageLayer = alpha * float(overlayEdges) * texture( imgCmapTex, imgCmapSlopeIntercept[0] * imgNorm + imgCmapSlopeIntercept[1] );

    // Apply color map to gradient magnitude:
    vec4 gradColormap = texture( imgCmapTex, imgCmapSlopeIntercept[0] * gradMag + imgCmapSlopeIntercept[1] );

    // For the edge layer, use either the solid edge color or the colormapped gradient magnitude:
    vec4 edgeLayer = alpha * mix( gradMag * edgeColor, gradColormap, float(colormapEdges) );

    // Look up label colors:
    vec4 segColor = texelFetch( segLabelCmapTex, int(seg), 0 ) * segOpacity * float(segMask);

    // Blend colors:
    OutColor = vec4( 0.0, 0.0, 0.0, 0.0 );
    OutColor = imageLayer + ( 1.0 - imageLayer.a ) * OutColor;
    OutColor = edgeLayer + ( 1.0 - edgeLayer.a ) * OutColor;
    OutColor = segColor + ( 1.0 - segColor.a ) * OutColor;
}
