#ifndef UI_CONTROLS
#define UI_CONTROLS

/**
 * @brief Settings for UI controls for a view or lightbox layout
 */
struct UiControls
{
    UiControls() = default;

    UiControls( bool visible )
        :
          m_hasImageComboBox( visible ),
          m_hasCameraTypeComboBox( visible ),
          m_hasShaderTypeComboBox( visible )
    {}

    bool m_hasImageComboBox = false;
    bool m_hasCameraTypeComboBox = false;
    bool m_hasShaderTypeComboBox = false;
};

#endif // UI_CONTROLS
