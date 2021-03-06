#version 330 core

#define MIN_IMAGE_TEXCOORD vec3( 0.0 )
#define MAX_IMAGE_TEXCOORD vec3( 1.0 )

#define N 2

in VS_OUT // Redeclared vertex shader outputs: now the fragment shader inputs
{
    vec3 ImgTexCoords[N];
    vec3 SegTexCoords[N];
} fs_in;

layout (location = 0) out vec4 OutColor; // Output RGBA color (pre-multiplied alpha)

uniform sampler3D imgTex[N]; // Texture units 0/1: images
uniform usampler3D segTex[N]; // Texture units 2/3: segmentations
uniform sampler1D segLabelCmapTex[N]; // Texutre unit 4/5: label color maps (pre-mult RGBA)

uniform vec2 imgSlopeIntercept[N]; // Slopes and intercepts for image window-leveling

uniform vec2 imgThresholds[N]; // Image lower and upper thresholds, mapped to OpenGL texture intensity
uniform float imgOpacity[N]; // Image opacities
uniform float segOpacity[N]; // Segmentation opacities

uniform bool magentaCyan;


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
    vec4 overlapColor = vec4( 0.0, 0.0, 0.0, 0.0 );
    vec4 segColor[2];

    for ( int i = 0; i < N; ++i )
    {
        // Foreground masks, based on whether texture coordinates are in range [0.0, 1.0]^3:
        bool imgMask = ! ( any(    lessThan( fs_in.ImgTexCoords[i], MIN_IMAGE_TEXCOORD ) ) ||
                           any( greaterThan( fs_in.ImgTexCoords[i], MAX_IMAGE_TEXCOORD ) ) );

        bool segMask = ! ( any(    lessThan( fs_in.SegTexCoords[i], MIN_IMAGE_TEXCOORD ) ) ||
                           any( greaterThan( fs_in.SegTexCoords[i], MAX_IMAGE_TEXCOORD ) ) );

        float val = texture( imgTex[i], fs_in.ImgTexCoords[i] ).r; // Image value
        uint label = texture( segTex[i], fs_in.SegTexCoords[i] ).r; // Label value
        float norm = clamp( imgSlopeIntercept[i][0] * val + imgSlopeIntercept[i][1], 0.0, 1.0 ); // Apply W/L

        // Apply opacity, foreground mask, and thresholds for images:
        float alpha = imgOpacity[i] * float(imgMask) * hardThreshold( val, imgThresholds[i] );

        // Look up label colors:
        segColor[i] = texelFetch( segLabelCmapTex[i], int(label), 0 ) * segOpacity[i] * float(segMask);

        overlapColor[i] = norm * alpha;
    }

    // Apply magenta/cyan option:
    overlapColor.b = float( magentaCyan ) * max( overlapColor.r, overlapColor.g );

    // Turn on overlap color if either the reference or moving image is present:
    // (note that this operation effectively thresholds out 0.0 as background)
    overlapColor.a = float( ( overlapColor.r > 0.0 ) || ( overlapColor.g > 0.0 ) );
    overlapColor.rgb = overlapColor.a * overlapColor.rgb;

    OutColor = segColor[0] + ( 1.0 - segColor[0].a ) * overlapColor;
    OutColor = segColor[1] + ( 1.0 - segColor[1].a ) * OutColor;
}
