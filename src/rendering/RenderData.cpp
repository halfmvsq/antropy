#include "rendering/RenderData.h"
#include "common/ParcellationLabelTable.h"

#include <spdlog/spdlog.h>


namespace
{

// Define the vertices and indices of a 2D quad:
static constexpr int sk_numQuadVerts = 4;
static constexpr int sk_numQuadPosComps = 2;

// Define the vertices and indices of a 2D circle:
//static constexpr int sk_numCircleVerts = 4;
//static constexpr int sk_numCirclePosComps = 2;

static constexpr int sk_byteOffset = 0;
static constexpr int sk_indexOffset = 0;

static const std::array< float, sk_numQuadVerts * sk_numQuadPosComps > sk_clipPosBuffer =
{ {
      -1.0f, -1.0f, // bottom left
      1.0f, -1.0f,  // bottom right
      -1.0f,  1.0f, // top left
      1.0f,  1.0f,  // top right
  } };

static const std::array< uint32_t, sk_numQuadVerts > sk_indicesBuffer = { { 0, 1, 2, 3 } };


GLTexture createBlankRGBATexture()
{
    static const ComponentType compType = ComponentType::UInt8;
    static std::array< uint8_t, 4 > sk_data_U8 = { 0, 0, 0, 0 };

    static constexpr GLint sk_mipmapLevel = 0; // Load image data into first mipmap level
    static constexpr GLint sk_alignment = 4; // Pixel pack/unpack alignment is 4 bytes

    static const tex::WrapMode sk_wrapMode = tex::WrapMode::ClampToEdge;
    static const tex::MinificationFilter sk_minFilter = tex::MinificationFilter::Nearest;
    static const tex::MagnificationFilter sk_maxFilter = tex::MagnificationFilter::Nearest;

    static const glm::uvec3 sk_size{ 1, 1, 1 };

    GLTexture::PixelStoreSettings pixelPackSettings;
    pixelPackSettings.m_alignment = sk_alignment;
    GLTexture::PixelStoreSettings pixelUnpackSettings = pixelPackSettings;

    GLTexture T( tex::Target::Texture3D,
                 GLTexture::MultisampleSettings(),
                 pixelPackSettings, pixelUnpackSettings );

    T.generate();
    T.setMinificationFilter( sk_minFilter );
    T.setMagnificationFilter( sk_maxFilter );
    T.setWrapMode( sk_wrapMode );
    T.setAutoGenerateMipmaps( false );
    T.setSize( sk_size );

    T.setData( sk_mipmapLevel,
               GLTexture::getSizedInternalRGBAFormat( compType ),
               GLTexture::getBufferPixelRGBAFormat( compType ),
               GLTexture::getBufferPixelDataType( compType ),
               sk_data_U8.data() );

    spdlog::debug( "Created blank RGBA texture" );

    return T;
}

} // anonymous


RenderData::RenderData()
    :
      m_quad(),
      m_circle(),

      m_imageTextures(),
      m_segTextures(),
      m_labelBufferTextures(),
      m_colormapTextures(),

      m_blankImageTexture( createBlankRGBATexture() ),
      m_blankSegTexture( createBlankRGBATexture() ),

      m_uniforms(),

      m_snapCrosshairsToReferenceVoxels( false ),
      m_maskedImages( false ),
      m_modulateSegOpacityWithImageOpacity( true ),

      m_backgroundColor( 0.1f, 0.1f, 0.1f ),
      m_crosshairsColor( 0.0f, 0.5f, 1.0f, 1.0f ),
      m_anatomicalLabelColor( 0.87f, 0.53f, 0.09f, 1.0f ),

      m_squaredDifferenceParams(),
      m_crossCorrelationParams(),
      m_jointHistogramParams(),

      m_edgeMagnitudeSmoothing( 1.0f, 1.0f ),
      m_numCheckerboardSquares( 10 ),
      m_overlayMagentaCyan( true ),
      m_quadrants( true, true ),
      m_useSquare( true ),
      m_flashlightRadius( 0.15f )
{
}


RenderData::Quad::Quad()
    :
      m_positionsInfo( BufferComponentType::Float,
                       BufferNormalizeValues::False,
                       sk_numQuadPosComps,
                       sk_numQuadPosComps * sizeof(float),
                       sk_byteOffset,
                       sk_numQuadVerts ),

      m_indicesInfo( IndexType::UInt32,
                     PrimitiveMode::TriangleStrip,
                     sk_numQuadVerts,
                     sk_indexOffset ),

      m_positionsObject( BufferType::VertexArray, BufferUsagePattern::StaticDraw ),
      m_indicesObject( BufferType::Index, BufferUsagePattern::StaticDraw ),

      m_vaoParams( m_indicesInfo )
{
    static constexpr GLuint sk_positionIndex = 0;

    m_positionsObject.generate();
    m_indicesObject.generate();

    m_positionsObject.allocate( sk_numQuadVerts * sk_numQuadPosComps * sizeof( float ), sk_clipPosBuffer.data() );
    m_indicesObject.allocate( sk_numQuadVerts * sizeof( uint32_t ), sk_indicesBuffer.data() );

    m_vao.generate();
    m_vao.bind();
    {
        m_indicesObject.bind(); // Bind EBO so that it is part of the VAO state

        // Saves binding in VAO, since GL_ARRAY_BUFFER is not part of VAO state.
        // Register position VBO with VAO and set/enable attribute pointer
        m_positionsObject.bind();
        m_vao.setAttributeBuffer( sk_positionIndex, m_positionsInfo );
        m_vao.enableVertexAttribute( sk_positionIndex );
    }
    m_vao.release();

    spdlog::debug( "Created image quad vertex array object" );
}


RenderData::Circle::Circle()
    :
      m_positionsInfo( BufferComponentType::Float,
                       BufferNormalizeValues::False,
                       sk_numQuadPosComps,
                       sk_numQuadPosComps * sizeof(float),
                       sk_byteOffset,
                       sk_numQuadVerts ),

      m_indicesInfo( IndexType::UInt32,
                     PrimitiveMode::TriangleStrip,
                     sk_numQuadVerts,
                     sk_indexOffset ),

      m_positionsObject( BufferType::VertexArray, BufferUsagePattern::StaticDraw ),
      m_indicesObject( BufferType::Index, BufferUsagePattern::StaticDraw ),

      m_vaoParams( m_indicesInfo )
{
    static constexpr GLuint sk_positionIndex = 0;

    m_positionsObject.generate();
    m_indicesObject.generate();

    m_positionsObject.allocate( sk_numQuadVerts * sk_numQuadPosComps * sizeof( float ), sk_clipPosBuffer.data() );
    m_indicesObject.allocate( sk_numQuadVerts * sizeof( uint32_t ), sk_indicesBuffer.data() );

    m_vao.generate();
    m_vao.bind();
    {
        m_indicesObject.bind(); // Bind EBO so that it is part of the VAO state

        // Saves binding in VAO, since GL_ARRAY_BUFFER is not part of VAO state.
        // Register position VBO with VAO and set/enable attribute pointer
        m_positionsObject.bind();
        m_vao.setAttributeBuffer( sk_positionIndex, m_positionsInfo );
        m_vao.enableVertexAttribute( sk_positionIndex );
    }
    m_vao.release();

    spdlog::debug( "Created image quad vertex array object" );
}
