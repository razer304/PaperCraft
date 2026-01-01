#include "VulkanBackend.h"

int main() {
    VulkanBackend app;

    try {
        app.runVulkanBackend();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}