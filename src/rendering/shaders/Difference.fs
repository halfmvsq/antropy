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
uniform sampler1D segLabelCmapTex[N]; // Texutre unit 6/7: label color tables (pre-mult RGBA)

uniform vec2 imgSlopeIntercept[N]; // Slopes and intercepts for image window-leveling
uniform float segOpacity[N]; // Segmentation opacities

uniform sampler1D metricCmapTex; // Texture unit 4: metric colormap (pre-mult RGBA)
uniform vec2 metricCmapSlopeIntercept; // Slope and intercept for the metric colormap
uniform vec2 metricSlopeIntercept; // Slope and intercept for the final metric
uniform bool metricMasking; // Whether to mask based on segmentation

uniform bool useSquare; // Whether to use squared difference (true) or absolute difference (false)


void main()
{
    float imgNorm[N];
    float mask[N];
    vec4 segColor[N];

    for ( int i = 0; i < N; ++i )
    {
        // Foreground masks, based on whether texture coordinates are in range [0.0, 1.0]^3:
        bool imgMask = ! ( any( lessThan( fs_in.ImgTexCoords[i], MIN_IMAGE_TEXCOORD ) ) ||
                           any( greaterThan( fs_in.ImgTexCoords[i], MAX_IMAGE_TEXCOORD ) ) );

        bool segMask = ! ( any( lessThan( fs_in.SegTexCoords[i], MIN_IMAGE_TEXCOORD ) ) ||
                           any( greaterThan( fs_in.SegTexCoords[i], MAX_IMAGE_TEXCOORD ) ) );

        float val = texture( imgTex[i], fs_in.ImgTexCoords[i] ).r; // Image value
        uint label = texture( segTex[i], fs_in.SegTexCoords[i] ).r; // Label value

        // Normalize images to [0.0, 1.0] range:
        imgNorm[i] = clamp( imgSlopeIntercept[i][0] * val + imgSlopeIntercept[i][1], 0.0, 1.0 );

        // Apply foreground mask and masking based on segmentation label:
        mask[i] = float( imgMask && ( metricMasking && ( label > 0u ) || ! metricMasking ) );

        // Look up label colors:
        segColor[i] = texelFetch( segLabelCmapTex[i], int(label), 0 ) * segOpacity[i] * float(segMask);
    }

    // Overall mask is the AND of both image masks:
    float metricMask = mask[0] * mask[1];

    // Compute metric and apply window/level to it:
    float metric = abs( imgNorm[0] - imgNorm[1] ) * metricMask;
    metric *= mix( 1.0, metric, float(useSquare) );

    metric = clamp( metricSlopeIntercept[0] * metric + metricSlopeIntercept[1], 0.0, 1.0 );

    // Index into colormap:
    float cmapValue = metricCmapSlopeIntercept[0] * metric + metricCmapSlopeIntercept[1];

    // Apply colormap and masking (by pre-multiplying RGBA with alpha mask):
    vec4 metricColor = texture( metricCmapTex, cmapValue ) * metricMask;

    // Ignore the segmentation layers if metric masking is enabled:
    float overlaySegs = float( ! metricMasking );
    segColor[0] = overlaySegs * segColor[0];
    segColor[1] = overlaySegs * segColor[1];

    // Blend colors:
    OutColor = vec4( 0.0, 0.0, 0.0, 0.0 );
    OutColor = segColor[0] + ( 1.0 - segColor[0].a ) * metricColor;
    OutColor = segColor[1] + ( 1.0 - segColor[1].a ) * OutColor;
}
