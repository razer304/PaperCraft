// InputHandler.h
#pragma once
#include <GLFW/glfw3.h>

class VulkanBackend; // forward declaration

class InputHandler {
public:
    explicit InputHandler(VulkanBackend* backend);

    void installCallbacks(GLFWwindow* window);

    // instance methods
    void onScroll(double xoffset, double yoffset);
    void onMouseButton(int button, int action, int mods);
    void onCursorMove(double xpos, double ypos);


    bool gDragging = false;

    double gLastX, gLastY;

    double xstart;
    double ystart;

    const double CLICK_THRESHOLD = 5.0;

    bool gPanning = false;
    float gYaw = 0.0f;   // rotation around Y axis
    float gPitch = 0.0f; // rotation around X axis
    double gLastPanX, gLastPanY;

    float gScale = 1.0f;



    float gPanX = 0.0f;
    float gPanY = 0.0f;



    


    // static GLFW callbacks
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);


private:
    VulkanBackend* backend;


    



    
};
