#include <cstdlib>
#include <cstring>
#include "esal_warmboot_api/esal_warmboot_api.h"

#include <iostream>
#include <fstream>
#include <string>
#include <sys/stat.h>

std::map<std::string, bool (*)()> warmBootRestoreHandlers = {
    {"VLAN",    vlanWarmBootRestoreHandler},
    {"PORT",    portWarmBootRestoreHandler},
    {"BRIDGE",  bridgeWarmBootRestoreHandler},
    {"TAG",     tagWarmBootRestoreHandler},
    {"STP",     stpWarmBootRestoreHandler},
    {"ACL",     aclWarmBootRestoreHandler},
};

std::map<std::string, bool (*)()> warmBootSaveHandlers = {
    {"VLAN",    vlanWarmBootSaveHandler},
    {"PORT",    portWarmBootSaveHandler},
    {"BRIDGE",  bridgeWarmBootSaveHandler},
    {"TAG",     tagWarmBootSaveHandler},
    {"STP",     stpWarmBootSaveHandler},
    {"ACL",     aclWarmBootSaveHandler},
};

std::map<std::string, void (*)()> warmBootCleanHandlers = {
    {"VLAN",    vlanWarmBootCleanHandler},
    {"PORT",    portWarmBootCleanHandler},
    {"BRIDGE",  bridgeWarmBootCleanHandler},
    {"TAG",     tagWarmBootCleanHandler},
    {"STP",     stpWarmBootCleanHandler},
    {"ACL",     aclWarmBootCleanHandler},
};

bool VendorWarmBootRestoreHandler() {
    bool status = true;
    bool rc = true;

    std::cout << "================================================================================" << std::endl;
    std::cout << "================= WarmBoot is running to restore configuration =================" << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << std::endl;

    for (auto handler_name_fn : warmBootRestoreHandlers) {
        std::string name = handler_name_fn.first;
        auto handler = handler_name_fn.second;


        std::cout << "WarmBoot handler of " << name << " is running..." << std::endl;
        rc = handler();

        std::cout << "================================================================================" << std::endl;
        if (rc) {
            std::cout << "OK" << std::endl;
        } else {
            std::cout << "Failed" << std::endl;
            status &= false;
        }
        std::cout << "================================================================================" << std::endl;
        std::cout << std::endl;
    }

    return status;
}

bool VendorWarmBootSaveHandler() {
    bool status = true;
    bool rc = true;

    std::cout << "================================================================================" << std::endl;
    std::cout << "================= WarmBoot is running to save configuration =================" << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << std::endl;

    if (!createFolderIfNotExist(BACKUP_FOLDER)) {
        std::cout << "Backup folder creation error" << std::endl;
        return false;
    }

    for (auto handler_name_fn : warmBootSaveHandlers) {
        std::string name = handler_name_fn.first;
        auto handler = handler_name_fn.second;

        std::cout << "WarmBoot handler of " << name << " is running..." << std::endl;
        rc = handler();

        if (rc) {
            std::cout << "OK" << std::endl;
        } else {
            std::cout << "Failed" << std::endl;
            status &= false;
        }
        std::cout << std::endl;
    }
    std::cout << "================================================================================" << std::endl;

    return status;
}

void VendorWarmBootCleanHanlder() {
    std::cout << "Clean modules state..." << std::endl;
    for (auto handler_name_fn : warmBootCleanHandlers) {
        std::string name = handler_name_fn.first;
        auto handler = handler_name_fn.second;

        std::cout << "Cleaning " << name << "state" << std::endl;
        handler();
    }
}

bool createFolderIfNotExist(const char *path) {
    struct stat st{};
    if (stat(path, &st) == 0) {
        return true;
    }

    std::string path_copy(path);
    auto last_slash = path_copy.find_last_of('/');
    if (last_slash != std::string::npos) {
        std::string new_dir(path_copy.substr(0, last_slash)); 
        std::cout << "createFolderIfNotExist create path: " << new_dir << "\n";
        bool parent_created = createFolderIfNotExist(new_dir.c_str());
        if (!parent_created) {
            return false;
        }
    }

    int status = mkdir(path, 0775);
    if (status != 0) {
        std::cout << "Error creating folder " << path << std::endl;
        return false;
    }
    return true;
}
