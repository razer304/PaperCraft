#include "VulkanBackend.h"
#include "inputhandler.h"




InputHandler::InputHandler(VulkanBackend* backend) : backend(backend) {}


void InputHandler::installCallbacks(GLFWwindow* window) {
    glfwSetWindowUserPointer(window, this); 

    glfwSetScrollCallback(window, scroll_callback);

    glfwSetMouseButtonCallback(window, mouse_button_callback);

    glfwSetCursorPosCallback(window, cursor_position_callback);

}



void InputHandler::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    auto* backend = reinterpret_cast<InputHandler*>(glfwGetWindowUserPointer(window));
    backend->onScroll(xoffset, yoffset);
}

void InputHandler::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    auto* backend = reinterpret_cast<InputHandler*>(glfwGetWindowUserPointer(window));
    backend->onMouseButton(button, action, mods);
}

void InputHandler::cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    auto* backend = reinterpret_cast<InputHandler*>(glfwGetWindowUserPointer(window));
    backend->onCursorMove(xpos, ypos);
}

void InputHandler::onScroll(double xoffset, double yoffset) {
    gScale += yoffset * 0.1f;
    if (gScale < 0.1f) gScale = 0.1f;
}


void InputHandler::onMouseButton(int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            gDragging = true;
            glfwGetCursorPos(backend->window, &gLastX, &gLastY);
            xstart = gLastX;
            ystart = gLastY;
        }
        else if (action == GLFW_RELEASE) {
            gDragging = false;

            double releaseX, releaseY;
            glfwGetCursorPos(backend->window, &releaseX, &releaseY);

            double dx = releaseX - xstart;
            double dy = releaseY - ystart;
            double dist = sqrt(dx * dx + dy * dy);

            if (dist < CLICK_THRESHOLD && backend->modelLoaded) {

                std::array<uint32_t, 2> edgeindex = backend->pickEdge(releaseX, releaseY);

                if (edgeindex[0] != -1) {

                    uint32_t i0 = backend->gMesh.unjoinedIndicesCPU[edgeindex[0]];
                    uint32_t i1 = backend->gMesh.unjoinedIndicesCPU[edgeindex[1]];


                    uint32_t* data;
                    vkMapMemory(backend->device, backend->gMesh.indexSelectorMemory, 0, VK_WHOLE_SIZE, 0, (void**)&data);


                    data[i0] = data[i0] ? 0 : 1;
                    data[i1] = data[i1] ? 0 : 1;

                    std::cout << "edge index hit point a!  " << i0 << std::endl;
                    std::cout << "edge index hit point b!  " << i1 << std::endl;

                    vkUnmapMemory(backend->device, backend->gMesh.indexSelectorMemory);
                }





            }
        }
    }

    if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        if (action == GLFW_PRESS) {
            gPanning = true;
            glfwGetCursorPos(backend->window, &gLastPanX, &gLastPanY);
        }
        else if (action == GLFW_RELEASE) {
            gPanning = false;
        }
    }
}

void InputHandler::onCursorMove(double xpos, double ypos) {
    if (gDragging) {
        double dx = xpos - gLastX;
        double dy = ypos - gLastY;
        gLastX = xpos;
        gLastY = ypos;

        gYaw += dx * 0.5f;
        gPitch += dy * 0.5f;
    }

    if (gPanning) {
        double dx = xpos - gLastPanX;
        double dy = ypos - gLastPanY;
        gLastPanX = xpos;
        gLastPanY = ypos;

        gPanX += dx * 0.01f;
        gPanY -= dy * 0.01f;
    }
}

