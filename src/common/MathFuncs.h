#ifndef MATH_FUNCS_H
#define MATH_FUNCS_H

#include "common/CoordinateFrame.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_precision.hpp>

#include <array>
#include <cmath>
#include <optional>
#include <string>
#include <utility>
#include <vector>


namespace math
{

/**
 * @brief generateRandomHsvSamples
 * @param numSamples Number of colors to generate
 * @param hueMinMax Min and max hue
 * @param satMinMax Min and max saturation
 * @param valMinMax Min and max value/intensity
 * @param seed
 * @return Vector of colors in HSV format
 */
std::vector< glm::vec3 > generateRandomHsvSamples(
        size_t numSamples,
        const std::pair< float, float >& hueMinMax,
        const std::pair< float, float >& satMinMax,
        const std::pair< float, float >& valMinMax,
        const std::optional<uint32_t>& seed );

/**
 * @brief Compute dimensions of image in Subject space
 *
 * @param[in] pixelDimensions Number of pixels per image dimensions
 * @param[in] pixelSpacing Pixel spacing in Subject space
 *
 * @return Vector of image dimensions in Subject space
 */
glm::dvec3 computeSubjectImageDimensions(
        const glm::u64vec3& pixelDimensions,
        const glm::dvec3& pixelSpacing );


/**
 * @brief Compute transformation from image Pixel space to Subject space.
 *
 * @param[in] directions Directions of image Pixel axes in Subject space
 * @param[in] pixelSpacing Pixel spacing in Subject space
 * @param[in] origin Image origin in Subject space
 *
 * @return 4x4 matrix transforming image Pixel to Subject space
 */
glm::dmat4 computeImagePixelToSubjectTransformation(
        const glm::dmat3& directions,
        const glm::dvec3& pixelSpacing,
        const glm::dvec3& origin );


/**
 * @brief Compute transformation from image Pixel space, with coordinates (i, j, k) representing
 * pixel indices in [0, N-1] range,  to image Texture coordinates (s, t, p) in [1/(2N), 1 - 1/(2N)] range
 *
 * @param[in] pixelDimensions Number of pixels per image dimensions
 *
 * @return 4x4 matrix transforming image Pixel to Texture space
 */
glm::dmat4 computeImagePixelToTextureTransformation(
        const glm::u64vec3& pixelDimensions );


/**
 * @brief computeInvPixelDimensions
 * @param pixelDimensions
 * @return
 */
glm::vec3 computeInvPixelDimensions( const glm::u64vec3& pixelDimensions );



std::array< glm::vec3, 8 > computeImagePixelAABBoxCorners(
        const glm::u64vec3& pixelDimensions );


/**
 * @brief Compute the bounding box of the image in physical Subject space.
 *
 * @param[in] pixelDimensions Number of pixels per image dimension
 * @param[in] directions Direction cosine matrix of image Pixel axes in Subject space
 * @param[in] pixelSpacing Pixel spacing in Subject space
 * @param[in] origin Image origin in Subject space
 *
 * @return Array of corners of the image bounding box in Subject space
 */
std::array< glm::vec3, 8 > computeImageSubjectBoundingBoxCorners(
        const glm::u64vec3& pixelDimensions,
        const glm::mat3& directions,
        const glm::vec3& pixelSpacing,
        const glm::vec3& origin );



std::pair< glm::vec3, glm::vec3 > computeMinMaxCornersOfAABBox(
        const std::array< glm::vec3, 8 >& subjectCorners );


/**
 * @brief Compute the corners of an axis-aligned bounding box with given min/max corners
 *
 * @param[in] boxMinMaxCorners Min/max corners of the AABB
 *
 * @return All eight AABB corners
 */
std::array< glm::vec3, 8 > computeAllAABBoxCornersFromMinMaxCorners(
        const std::pair< glm::vec3, glm::vec3 >& boxMinMaxCorners );


/**
 * @brief Compute the anatomical direction "SPIRAL" code of an image from its direction matrix
 *
 * @param[in] directions 3x3 direction matrix: columns are the direction vectors
 *
 * @return Pair of the three-letter direction code and boolean flag that is true when the directions are oblique
 * to the coordinate axes
 *
 * @todo SPIRAL CODE IS WRONG FOR hippo warp image
 */
std::pair< std::string, bool > computeSpiralCodeFromDirectionMatrix(
        const glm::dmat3& directions );


/**
 * @brief Apply rotation to a coordinate frame about a given world center position
 * @param frame Frame to rotate
 * @param rotation Rotation, expressed as a quaternion
 * @param worldCenter Center of rotation in World space
 */
void rotateFrameAboutWorldPos(
        CoordinateFrame& frame,
        const glm::quat& rotation,
        const glm::vec3& worldCenter );

} // namespace math

#endif // MATH_FUNCS_H
