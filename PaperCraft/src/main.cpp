#include "VulkanBackend.h"

int main() {
    VulkanBackend backvulk;


    try {
        backvulk.runVulkanBackend();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}