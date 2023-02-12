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

// Defenition of warmboot restore handler functions
//
extern "C" bool vlanWarmBootRestoreHandler();
extern "C" bool portWarmBootRestoreHandler();
extern "C" bool bridgeWarmBootRestoreHandler();

// Warmboot restore handlers
//
extern std::map<std::string, bool (*)()> warmBootRestoreHandlers;


// Definition of warmboot save handler functions
//
extern "C" bool vlanWarmBootSaveHandler();
extern "C" bool portWarmBootSaveHandler();
extern "C" bool bridgeWarmBootSaveHandler();

// Warmboot save handlers
//
extern std::map<std::string, bool (*)()> warmBootSaveHandlers;

// Warmboot handlers
//
extern "C" bool VendorWarmBootRestoreHandler();
extern "C" bool VendorWarmBootSaveHandler();

#endif //WARMBOOT_DEFS_H
