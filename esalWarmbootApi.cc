#include <esal_warmboot_api/esal_warmboot_api.h>

#include <iostream>
#include <fstream>
#include <string>
#include <sys/stat.h>

bool ESAL_WARM = true;

std::map<std::string, bool (*)()> warmBootHandlers = {
    {"VLAN", vlanWarmBootHandler},
};

bool esalWarmBootHandler() {
    bool status = true;

    std::cout << "================================================================================" << std::endl;
    std::cout << "================= WarmBoot is running to restore configuration =================" << std::endl;
    std::cout << "================================================================================" << std::endl;

    for (auto handler_name_fn : warmBootHandlers) {
        std::string name = handler_name_fn.first;
        auto handler = handler_name_fn.second;

        std::cout << "WarmBoot handler of " << name << " is running..." << std::endl;
        status = handler();
        std::cout << (status ? "OK" : "Failed") << std::endl;
        std::cout << std::endl;
    }
    std::cout << "================================================================================" << std::endl;

    return status;
}

bool createFolderIfNotExist(const char *path) {
    struct stat st{};
    if (stat(path, &st) == 0) {
        return true;
    }

    int status = mkdir(path, 0775);
    if (status != 0) {
        std::cout << "Error creating folder " << path << std::endl;
        return false;
    }
    return true;
}
