#ifndef WARMBOOT_DEFS_H
#define WARMBOOT_DEFS_H

#include <stdint.h>
#include <vector>
#include <map>
#include <string>

#define BACKUP_FOLDER "/var/shared/esal/esalbase-warm-restart"

#define BACKUP_FILE_VLAN                    BACKUP_FOLDER   "wb_vlan"
#define BACKUP_FILE_PORT                    BACKUP_FOLDER   "wb_port"
#define BACKUP_FILE_BRIDGE                  BACKUP_FOLDER   "wb_bridge"
#define BACKUP_FILE_TAG                     BACKUP_FOLDER   "wb_tag"
#define BACKUP_FILE_STP                     BACKUP_FOLDER   "wb_stp"
#define BACKUP_FILE_PORT_TRANS_MAP_ING      BACKUP_FOLDER   "wb_port_trans_map_ing"
#define BACKUP_FILE_PORT_TRANS_MAP_EGR      BACKUP_FOLDER   "wb_port_trans_map_egr"
#define BACKUP_FILE_PORT_ACL_ING            BACKUP_FOLDER   "wb_port_acl_ing"
#define BACKUP_FILE_PORT_ACL_EGR            BACKUP_FOLDER   "wb_port_acl_egr"

// Warmboot flag
//
extern bool WARM_RESTART;

// Warmboot helpers
//
bool createFolderIfNotExist(const char *path);

// Defenition of warmboot restore handler functions
//
extern "C" bool vlanWarmBootRestoreHandler();
extern "C" bool portWarmBootRestoreHandler();
extern "C" bool bridgeWarmBootRestoreHandler();
extern "C" bool tagWarmBootRestoreHandler();
extern "C" bool stpWarmBootRestoreHandler();
extern "C" bool aclWarmBootRestoreHandler();

// Warmboot restore handlers
//
extern std::map<std::string, bool (*)()> warmBootRestoreHandlers;


// Definition of warmboot save handler functions
//
extern "C" bool vlanWarmBootSaveHandler();
extern "C" bool portWarmBootSaveHandler();
extern "C" bool bridgeWarmBootSaveHandler();
extern "C" bool tagWarmBootSaveHandler();
extern "C" bool stpWarmBootSaveHandler();
extern "C" bool aclWarmBootSaveHandler();

// Warmboot save handlers
//
extern std::map<std::string, bool (*)()> warmBootSaveHandlers;

// Definition of warmboot clean handler functions
//
extern "C" void vlanWarmBootCleanHandler();
extern "C" void portWarmBootCleanHandler();
extern "C" void bridgeWarmBootCleanHandler();
extern "C" void tagWarmBootCleanHandler();
extern "C" void stpWarmBootCleanHandler();
extern "C" void aclWarmBootCleanHandler();

// Warmboot clean handlers
//
extern std::map<std::string, void (*)()> warmBootCleanHandlers;

// Warmboot handlers
//
extern "C" bool VendorWarmBootRestoreHandler();
extern "C" bool VendorWarmBootSaveHandler();
extern "C" void VendorWarmBootCleanHanlder();

#endif //WARMBOOT_DEFS_H
