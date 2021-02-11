#ifndef GLFW_CALLBACKS_H
#define GLFW_CALLBACKS_H

struct GLFWwindow;

void errorCallback( int error, const char* description );
void windowContentScaleCallback( GLFWwindow* window, float fbToWinScaleX, float fbToWinScaleY );
void windowCloseCallback( GLFWwindow* window );
void windowSizeCallback( GLFWwindow* window, int winWidth, int winHeight );
void cursorPosCallback( GLFWwindow* window, double mousePosX, double mousePosY );
void mouseButtonCallback( GLFWwindow* window, int button, int action, int mods );
void scrollCallback( GLFWwindow* window, double xoffset, double yoffset );
void keyCallback( GLFWwindow* window, int key, int scancode, int action, int mods );
void dropCallback( GLFWwindow* window, int count, const char** paths );

#endif // GLFW_CALLBACKS_H
