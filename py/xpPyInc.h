// xpPyInc.h

/*******************************************************************************
* Copyright (c) 2021 Marvell. All rights reserved. The following file is       *
* subject to the limited use license agreement by and between Marvell and you, *
* your employer or other entity on behalf of whom you act. In the absence of   *
* such license agreement the following file is subject to Marvell’s standard   *
* Limited Use License Agreement.                                               *
********************************************************************************/

#ifndef XP_PY_INC_H
#define XP_PY_INC_H
#include "pthread.h"
#include "xpTypes.h"

#define XP_PY_INPUT_BUF_LEN       (512)             ///< Input buffer length
#define XPSHELL_MAX_SESSIONS      (8)               ///< Maximum sessions for xpshell
#define XPSHELL_MAX_PIPE_NAME     (256)             ///< maximum pipe name xpshell
#define XP_CONFIG_TRANS_FILE      "/tmp/xpConflg"

/* Used to assign and free Session-Id*/
#define ENTRY_POINT 0xA0    //Used to assign the SessionId to Client when client wants to join the server and expect the new SessionId; Example:: SessionId + 0xA0
#define EXIT_POINT 0xE0    //Used to free the assigned Session-Id when client wants to leave the Server; Example:: SessionID + 0xE0
#define NEGATIVE_FEEDBACK 0xEE //Used to give the feedback to client when requested Session-Id is not available at Server-Side

/*SessionId range is 0-7 & Total available Sessions are 8 */
#define MIN_CLIENT_ID 0
#define MAX_CLIENT_ID 7
#define USER_ROOT "root"
#define USER_ADMIN "admin"


/* xp shell clients */
typedef enum
{
    XP_SHELL_NONE,
    XP_SHELL_APP,
    XP_SHELL_SIM,
    XP_SHELL_DEFAULT,
    XP_SHELL_STANDALONE,
    XP_SHELL_CLIENT,
    XP_SHELL_KERNEL,
    XP_SHELL_INLINE,
    XP_SHELL_MAX

} XP_SHELL_CLIENT_ID;

/* xp shell exit modes */
typedef enum
{
    XP_SHELL_EXIT_0,
    XP_SHELL_EXIT_1,
    XP_SHELL_EXIT_2,
    XP_SHELL_EXIT_3,
    XP_SHELL_EXIT_4,
    XP_SHELL_EXIT_5,
    XP_SHELL_EXIT_6,
    XP_SHELL_EXIT_LAST
} xpShellExitMode_e;

typedef struct shellArg
{
    int clientId;           ///< clientId
    char *configFile;       ///< configFile
} xpShellArg;

/**
 * \brief change file owner and group for absolute file path
 *
 * \param [in] file_path absolute file path
 * \param [in] user_name root
 * \param [in] group_name admin
 *
 * \return XP_STATUS on success XP_NO_ERR
 **/
XP_STATUS doChown(const char *file_path, const char *user_name,
                  const char *group_name);

#if defined (__cplusplus)
extern "C" {
#endif
extern uint8_t shellRun;
extern uint8_t pyInput[];
extern XP_STATUS xpPyInit(void);
extern XP_STATUS xpPyInvoke(int task, XP_SHELL_CLIENT_ID client,
                            char *confFile);
extern XP_STATUS xpPyForceInvoke(int task, XP_SHELL_CLIENT_ID client,
                                 char *confFile);
extern XP_STATUS setPyInput(int i, uint8_t value);
extern uint8_t getPyInput(int i);
extern void xpShellExit(int val);
extern XP_STATUS void_tp_setitem(void *addr, int i, uint8_t value);
extern uint8_t void_tp_getitem(void *addr, int i);
extern XP_STATUS macAddr_tp_setitem(macAddr_t *addr, int i, uint8_t value);
extern uint8_t macAddr_tp_getitem(macAddr_t *addr, int i);
extern XP_STATUS ipv4Addr_tp_setitem(ipv4Addr_t *addr, int i, uint8_t value);
extern uint8_t ipv4Addr_tp_getitem(ipv4Addr_t *addr, int i);
extern XP_STATUS ipv6Addr_tp_setitem(ipv6Addr_t *addr, int i, uint8_t value);
extern uint8_t ipv6Addr_tp_getitem(ipv6Addr_t *addr, int i);
extern XP_STATUS compIpv6Addr_tp_setitem(compIpv6Addr_t *addr, int i,
                                         uint8_t value);
extern uint8_t compIpv6Addr_tp_getitem(compIpv6Addr_t *addr, int i);
extern XP_STATUS macAddrHigh_tp_setitem(macAddrHigh_t *addr, int i,
                                        uint8_t value);
extern uint8_t macAddrHigh_tp_getitem(macAddrHigh_t *addr, int i);

extern XP_STATUS pyDalInit();
extern XP_STATUS pyDalWrite(int sessionId, int dir);
extern XP_STATUS pyDalRead(int *sessionId, int dir);
extern XP_STATUS pyDalDeInit();
extern XP_STATUS xpPyCliRequests(int sessionId);

#if defined (__cplusplus)
}
#endif

#endif

