#ifndef WARMBOOT_DEFS_H
#define WARMBOOT_DEFS_H

#include <stdint.h>
#include <vector>
#include <map>
#include <string>

#include <libconfig.h++>

#define BACKUP_FOLDER "wb_backup/"

#define BACKUP_FILE_VLAN BACKUP_FOLDER "wb_vlan"

// Warmboot flag
//
extern bool ESAL_WARM;

// Warmboot helpers
//
uint32_t calculateCRC(uint8_t *data, int size);
bool createFolderIfNotExist(const char *path);

template <typename T>
void vecWriteToFile(std::vector<T>& data, const std::string& filename);

template <typename T>
void vecReadFromFile(std::vector<T>& data, const std::string& filename);

// Defenition of warmboot handler functions
//
extern "C" bool vlanWarmBootHandler();

// Warmboot handlers
//
extern std::map<std::string, bool (*)()> warmBootHandlers;

// Warmboot runner
//
bool esalWarmBootHandler();

#endif //WARMBOOT_DEFS_H
