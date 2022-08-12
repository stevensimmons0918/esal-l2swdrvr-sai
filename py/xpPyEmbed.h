// xpPyEmbed.h

/*******************************************************************************
* Copyright (c) 2021 Marvell. All rights reserved. The following file is       *
* subject to the limited use license agreement by and between Marvell and you, *
* your employer or other entity on behalf of whom you act. In the absence of   *
* such license agreement the following file is subject to Marvell’s standard   *
* Limited Use License Agreement.                                               *
********************************************************************************/

#ifndef XP_PY_EMBED_H
#define XP_PY_EMBED_H
#include "xpTypes.h"

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

