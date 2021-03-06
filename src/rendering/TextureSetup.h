#ifndef TEXTURE_SETUP_H
#define TEXTURE_SETUP_H

#include "rendering/utility/gl/GLTexture.h"

#include <uuid.h>
#include <unordered_map>

class AppData;

std::unordered_map< uuids::uuid, std::vector<GLTexture> >
createImageTextures( const AppData& appData );

std::unordered_map< uuids::uuid, GLTexture >
createSegTextures( const AppData& appData );

std::unordered_map< uuids::uuid, GLTexture >
createImageColorMapTextures( const AppData& appData );

std::unordered_map< uuids::uuid, GLTexture >
createLabelColorTableTextures( const AppData& appData );

#endif // TEXTURE_SETUP_H
