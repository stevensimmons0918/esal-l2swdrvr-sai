#ifndef WARMBOOT_DEFS_H
#define WARMBOOT_DEFS_H

#include <stdint.h>
#include <vector>
#include <map>
#include <string>

#define BACKUP_FOLDER "wb_backup/"

#define BACKUP_FILE_VLAN    BACKUP_FOLDER   "wb_vlan"
#define BACKUP_FILE_PORT    BACKUP_FOLDER   "wb_port"
#define BACKUP_FILE_BRIDGE  BACKUP_FOLDER   "wb_bridge"

// Warmboot flag
//
extern bool ESAL_WARM;

// Warmboot helpers
//
bool createFolderIfNotExist(const char *path);

// Defenition of warmboot handler functions
//
extern "C" bool vlanWarmBootHandler();
extern "C" bool portWarmBootHandler();
extern "C" bool bridgeWarmBootHandler();

// Warmboot handlers
//
extern std::map<std::string, bool (*)()> warmBootHandlers;

// Warmboot runner
//
bool esalWarmBootHandler();

#endif //WARMBOOT_DEFS_H
