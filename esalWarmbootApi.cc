#include <esal_warmboot_api/esal_warmboot_api.h>

#include <iostream>
#include <fstream>
#include <string>
#include <sys/stat.h>

bool ESAL_WARM = true;

std::map<std::string, bool (*)()> warmBootHandlers = {
    {"Vlan warm boot handler", vlanWarmBootHandler},
};

bool esalWarmBootHandler() {
    bool status = true;

    std::cout << "================================================================================" << std::endl;    
    std::cout << "================= WarmBoot is running to restore configuration =================" << std::endl;    
    std::cout << "================================================================================" << std::endl;    

    for (auto handler_name_fn : warmBootHandlers) {
        std::string name = handler_name_fn.first;
        auto handler = handler_name_fn.second;

        std::cout << "WarmBoot handler of " << name << "is running..." << std::endl;    
        status = handler();
        std::cout << (status ? "OK" : "Failed") << std::endl;
        std::cout << std::endl;
    }
    std::cout << "================================================================================" << std::endl;    

    return status;
}

uint32_t calculateCRC(uint8_t *data, int size) {
    uint32_t crc = 0;

    for (int i = 0; i < size; i++) {
        crc ^= data[i];

        for (int j = 0; j < 8; j++) {
            if (crc & 0x00000001) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

template <typename T>
bool vecWriteToFile(std::vector<T>& data, const std::string& filename) {
    std::ofstream out(filename, std::ios::binary);

    if (!out.is_open()) {
        std::cout << "File opening error: " << filename << std::endl;
        return false;
    } 

    auto crc = calculateCRC((uint8_t *)data.data(), data.size() * sizeof(T));
    out.write((char*)&crc, sizeof(crc));
 
    uint32_t size = data.size();
    out.write((char*)&size, sizeof(size));

    for (const auto& d : data) {
        out.write((char*)&d, sizeof(d));
    }

    out.close();

    return true;
}

template <typename T>
bool vecReadFromFile(std::vector<T>& data, const std::string& filename) {
    std::ifstream in(filename, std::ios::binary);

    if (!in.is_open()) {
        std::cout << "File opening error: " << filename << std::endl;
        return false;
    }

    uint32_t crc;
    in.read((char*)&crc, sizeof(crc));

    uint32_t size;
    in.read((char*)&size, sizeof(size));

    data.resize(size);
    for (auto& d : data) {
        in.read((char*)&d, sizeof(d));
    }

    uint32_t calcCrc = calculateCRC((uint8_t *)data.data(), data.size() * sizeof(T));
    if (crc != calcCrc) {
        std::cout << "CRC check failed: " << filename << std::endl;
        return false;
    } else {
        std::cout << "CRC OK!" << std::endl;
    }

    in.close();

    return true;
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
