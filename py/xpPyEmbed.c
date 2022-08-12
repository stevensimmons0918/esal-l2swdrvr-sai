// xpPyEmbed.c

/*******************************************************************************
* Copyright (c) 2021 Marvell. All rights reserved. The following file is       *
* subject to the limited use license agreement by and between Marvell and you, *
* your employer or other entity on behalf of whom you act. In the absence of   *
* such license agreement the following file is subject to Marvell’s standard   *
* Limited Use License Agreement.                                               *
********************************************************************************/

#include "Python.h"
#include "xpPyInc.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>

#ifndef MAC
pthread_t gSigThread = 0;
#endif
/**
 ** \fn pyInput
 ** \public
 ** \brief get xp reg info structure
 **
 ** \return None
 **/
uint8_t pyInput[XP_PY_INPUT_BUF_LEN];
uint8_t shellRun = XP_SHELL_EXIT_0;
static FILE* PythonScriptFile = NULL;
static xpShellArg taskArg;
#define PY_INIT_MOD_NAME  "emb"
static pthread_t gShThread = 0;
static uint8_t threadCnt = 0;
static uint8_t pyInitDone = 0;

static pthread_t srvThread;
static bool srvThreadExists = false;

static const char* pyScriptLocations[] =
{
    ABSOLUTE_PATH "/cli/test.py",
    "./../../cli/xpShell.py",
    "./../../../../cli/xpShell.py",
    "./../../../../../cli/xpShell.py",
    "./../cli/xpShell.py",
    "./cli/test.py"
};

XP_STATUS doChown(const char *filePath, const char *userName,
                  const char *groupName)
{
    int retVal = 0;
    uid_t uid;
    gid_t gid;
    struct passwd pwd;
    struct passwd *resultPwd;
    struct group grp;
    struct group  *resultGroup;

    /* allocate a buffer of sufficient size */
    long int initlen = sysconf(_SC_GETPW_R_SIZE_MAX);
    size_t len;
    char *bufferPasswd = NULL;
    char *bufferGroup = NULL;
    if (initlen == -1)
    {
        /* Default initial length. */
        len = 1024;
    }
    else
    {
        len = (size_t) initlen;
    }
    bufferPasswd = (char *)malloc(len);
    if (NULL == bufferPasswd)
    {
        printf("%s():%d: malloc failed with Error : %s\n", __FUNCTION__, __LINE__,
               strerror(errno));
        return XP_ERR_NULL_POINTER;
    }

    /* get uid */
    getpwnam_r(userName, &pwd, bufferPasswd, len, &resultPwd);
    if (NULL == resultPwd)
    {
        printf("%s():%d: getpwnam_r failed with Error : %s\n", __FUNCTION__, __LINE__,
               strerror(errno));
        free(bufferPasswd);
        return XP_ERR_NULL_POINTER;
    }
    uid = pwd.pw_uid;

    /* get gid */
    bufferGroup = (char *)malloc(len);
    if (NULL == bufferGroup)
    {
        printf("%s():%d: malloc failed with Error : %s\n", __FUNCTION__, __LINE__,
               strerror(errno));
        free(bufferPasswd);
        return XP_ERR_NULL_POINTER;
    }
    retVal = getgrnam_r(groupName, &grp, bufferGroup, len, &resultGroup);
    if (NULL == resultGroup)
    {
        // User is Admin, but Admin group is not present
        retVal = getpwnam_r(groupName, &pwd, bufferPasswd, len, &resultPwd);
        if (NULL == resultPwd)
        {
            printf("%s():%d: getpwnam_r failed with Error:%d ret:%d\n", __FUNCTION__,
                   __LINE__, errno, retVal);
            free(bufferPasswd);
            free(bufferGroup);
            return XP_ERR_NULL_POINTER;
        }
        else
        {
            gid = pwd.pw_gid;
        }
    }
    else
    {
        gid = grp.gr_gid;
    }

    /* change file ownership */
    if (chown(filePath, uid, gid) == -1)
    {
        printf("%s():%d: chown failed with Error : %s\n", __FUNCTION__, __LINE__,
               strerror(errno));
        free(bufferPasswd);
        free(bufferGroup);
        return XP_ERR_INVALID_ARG;
    }

    free(bufferPasswd);
    free(bufferGroup);
    return XP_NO_ERR;
}


XP_STATUS xpPyInit()
{
    int argc = 0;
    char *argv[1] = { NULL };

    if (pyInitDone == 1)
    {
        return XP_NO_ERR;
    }
    pyInitDone = 1;

    if (Py_IsInitialized())
    {
        if (PythonScriptFile != NULL)
        {
            fclose(PythonScriptFile);
            PythonScriptFile = NULL;
        }
        Py_Finalize();
    }
    pyDalInit();
    Py_Initialize();
    PyEval_InitThreads();
    PyEval_ReleaseLock();
    PySys_SetArgv(argc, argv);
    Py_InitModule(PY_INIT_MOD_NAME, NULL);

    return (XP_NO_ERR);
}

static void sig_handler(int signo)
{
    printf("Exiting primary xpShell\n");
    xpShellExit(4);
}

static FILE* xpPyGetScriptFd(char *filePath)
{
    char *xpRootPath = getenv("XP_ROOT");

    if (xpRootPath == NULL)
    {
        xpRootPath = getenv(XP_SAI_CONFIG_PATH);
    }



    if ((xpRootPath != NULL) && (strlen(xpRootPath) > 1))
    {
        if (xpRootPath[strlen(xpRootPath) - 1] != '/')
        {
            sprintf(filePath, "%s%s", xpRootPath, "/cli/xpShell.py");
        }
        else
        {
            sprintf(filePath, "%s%s", xpRootPath, "cli/xpShell.py");
        }

        FILE* pyScriptFd = fopen(filePath, "r");
        if (pyScriptFd != NULL)
        {
            return pyScriptFd;
        }
    }

    int idx = 0, size = sizeof(pyScriptLocations)/sizeof(const char*);

    for (idx = 0; idx < size; idx++)
    {
        sprintf(filePath, "%s", pyScriptLocations[idx]);
        FILE* pyScriptFd = fopen(filePath, "r");
        if (pyScriptFd != NULL)
        {
            return pyScriptFd;
        }
    }

    return NULL;
}

void* xpPyInvokeTask(void* arg)
{
    int argc;
    char * argv[6];
    char cmdPath[256], opt1[4], opt2[4];
    char filePath[2048];
    char execName[256];
    xpShellArg *taskArg = (xpShellArg *)arg;
    PyGILState_STATE gstate;
    XP_SHELL_CLIENT_ID clientId = (XP_SHELL_CLIENT_ID)(taskArg->clientId);
    char *xpShellArgs = NULL;

    gstate = PyGILState_Ensure();

    if (PythonScriptFile != NULL)
    {
        fclose(PythonScriptFile);
        PythonScriptFile = NULL;
    }

    xpShellArgs = getenv("XP_SHELL_ARGS");
    if (xpShellArgs == NULL)
    {
        xpShellArgs = (char*)"-t crb2-rev2 -m 100G -M INTR";
    }

    PythonScriptFile = xpPyGetScriptFd(filePath);

    if (PythonScriptFile != NULL)
    {
        if (clientId == XP_SHELL_SIM)
        {
            argc = 2;
            argv[0] = (char*)"xpShell.py";
            argv[1] = (char*)"--xpSim";
            if (taskArg->configFile != NULL)
            {
                sprintf(execName, "-n %s", taskArg->configFile);
                argc = 3;
                argv[2] = execName;
            }
            Py_SetProgramName(argv[0]);
            PySys_SetArgv(argc, argv);
        }
        else
            // python cli/xpShell.py --kernel -x "-t crb2-rev2 -m 100G -M INTR" -n /tmp/tmpXpShell.txt
            // -s XP_FDB_128K_TWO_PIPE_PROFILE"
            if ((clientId == XP_SHELL_KERNEL) || (clientId == XP_SHELL_INLINE))
            {
                argc = 2;
                argv[0] = (char*)"xpShell.py";
                argv[1] = (char*)"--inmode";
                char *kernelMode = (char*)"--kernel";

                if (clientId == XP_SHELL_KERNEL)
                {
                    argv[1] = kernelMode;
                }

                if (taskArg->configFile == NULL)
                {
                    sprintf(filePath, "-x '%s'", xpShellArgs);
                    argc = 3;
                    argv[2] = filePath;
                }
                else
                {
                    sprintf(opt1, "-x");
                    argc = 3;
                    argv[2] = opt1;

                    sprintf(cmdPath, "%s", xpShellArgs);
                    argc = 4;
                    argv[3] = cmdPath;

                    sprintf(opt2, "-n");
                    argc = 5;
                    argv[4] = opt2;

                    sprintf(filePath, "%s", taskArg->configFile);
                    argc = 6;
                    argv[5] = filePath;
                }
                Py_SetProgramName(argv[0]);
                PySys_SetArgv(argc, argv);
            }
            else if (clientId == XP_SHELL_STANDALONE)
            {
                argc = 2;
                argv[0] = (char*)"xpShell.py";
                argv[1] = (char*)"--standalone";
                if (taskArg->configFile != NULL)
                {
                    sprintf(execName, "-n %s", taskArg->configFile);
                    argc = 3;
                    argv[2] = execName;
                }
                Py_SetProgramName(argv[0]);
                PySys_SetArgv(argc, argv);
            }
            else if (clientId == XP_SHELL_CLIENT)
            {
                argc = 2;
                argv[0] = (char*)"xpShellClient.py";
                argv[1] = (char*)"--default";
                if (taskArg->configFile != NULL)
                {
                    sprintf(execName, "-n %s", taskArg->configFile);
                    argc = 3;
                    argv[2] = execName;
                }
                Py_SetProgramName(argv[0]);
                PySys_SetArgv(argc, argv);
            }
            else
            {
                argc = 2;
                argv[0] = (char*)"xpShell.py";
                argv[1] = (char*)"--default";
                if (taskArg->configFile != NULL)
                {
                    sprintf(execName, "-n %s", taskArg->configFile);
                    argc = 3;
                    argv[2] = execName;
                }
                if (signal(SIGUSR1, sig_handler) == SIG_ERR)
                {
                    printf("Error in SIGINT\n");
                }
                Py_SetProgramName(argv[0]);
                PySys_SetArgv(argc, argv);
            }
        PyRun_SimpleFile(PythonScriptFile, filePath);
    }
    PyGILState_Release(gstate);
    return NULL;
}

XP_STATUS xpPyInvoke(int task, XP_SHELL_CLIENT_ID client, char *confFile)
{
    pthread_t thread1 = 0;
    int created;

    xpPyInit();

    if ((!task) && (client != XP_SHELL_CLIENT))
    {
        shellRun = XP_SHELL_EXIT_0;
    }
    if (shellRun != XP_SHELL_EXIT_0)
    {
        /* shell is already running */
        return (XP_NO_ERR);
    }
    taskArg.clientId = (int)client;
    taskArg.configFile = confFile;

    if (!task)
    {
        xpPyInvokeTask((void *)&taskArg);
        created = 0;
    }
    else
    {
        created = pthread_create(&thread1, NULL, xpPyInvokeTask, (void *)&taskArg);
    }

    if (created != 0) // => error happened
    {
        int errcode = errno;
        printf("Error creating python shell: %s\n", strerror(errcode));
        return XP_ERR_FILE_OPEN;
    }
    else if (client != XP_SHELL_CLIENT)
    {
        gShThread = thread1;
        threadCnt = 0;
    }

    if (task)
    {
        shellRun = XP_SHELL_EXIT_1;
    }

    return (XP_NO_ERR);
}

XP_STATUS xpPyForceInvoke(int task, XP_SHELL_CLIENT_ID client, char *confFile)
{
    shellRun = XP_SHELL_EXIT_0;
    return (xpPyInvoke(task, client, confFile));
}


XP_STATUS setPyInput(int i, uint8_t value)
{
    if (i >= (XP_PY_INPUT_BUF_LEN-1))
    {
        printf("Error, the length greater than %d is not supported yet\n", i);
        return (XP_ERR_INVALID_ARG);
    }
    pyInput[i] = value;
    pyInput[i+1] = '\0';
    return (XP_NO_ERR);
}


uint8_t getPyInput(int i)
{
    if (i >= (XP_PY_INPUT_BUF_LEN-1))
    {
        printf("Error, the length greater than %d is not supported yet\n", i);
        return (0);
    }
    return (pyInput[i]);
}


XP_STATUS void_tp_setitem(void *addr, int i, uint8_t value)
{
    uint8_t *ptr = (uint8_t *)addr;
    ptr[i] = value;
    return (XP_NO_ERR);
}


uint8_t void_tp_getitem(void *addr, int i)
{
    uint8_t *ptr = (uint8_t *)addr;
    uint8_t value = ptr[i];
    return (value);
}

XP_STATUS macAddr_tp_setitem(macAddr_t *addr, int i, uint8_t value)
{
    return (void_tp_setitem(addr, i, value));
}


uint8_t macAddr_tp_getitem(macAddr_t *addr, int i)
{
    return (void_tp_getitem(addr, i));
}


XP_STATUS ipv4Addr_tp_setitem(ipv4Addr_t *addr, int i, uint8_t value)
{
    return (void_tp_setitem(addr, i, value));
}


uint8_t ipv4Addr_tp_getitem(ipv4Addr_t *addr, int i)
{
    return (void_tp_getitem(addr, i));
}

XP_STATUS ipv6Addr_tp_setitem(ipv6Addr_t *addr, int i, uint8_t value)
{
    return (void_tp_setitem(addr, i, value));
}


uint8_t ipv6Addr_tp_getitem(ipv6Addr_t *addr, int i)
{
    return (void_tp_getitem(addr, i));
}

XP_STATUS compIpv6Addr_tp_setitem(compIpv6Addr_t *addr, int i, uint8_t value)
{
    return (void_tp_setitem(addr, i, value));
}


uint8_t compIpv6Addr_tp_getitem(compIpv6Addr_t *addr, int i)
{
    return (void_tp_getitem(addr, i));
}

XP_STATUS macAddrHigh_tp_setitem(macAddrHigh_t *addr, int i, uint8_t value)
{
    return (void_tp_setitem(addr, i, value));
}


uint8_t macAddrHigh_tp_getitem(macAddrHigh_t *addr, int i)
{
    return (void_tp_getitem(addr, i));
}

void xpShellExit(int val)
{
    // int ret = 3;

    /* if val = 0 then it relaunches */
    shellRun = (xpShellExitMode_e)val;
    gShThread = 0;
    threadCnt = 0;

    if (val == 4)
    {
        shellRun = XP_SHELL_EXIT_2;
        printf("Exiting thread\n");
        //pthread_exit(&ret);
        printf("Done\n");
    }

    /* exception handler from shell */
    if (val == 3)
    {
        raise(SIGUSR2);
    }
}

const char *xpGetUserName()
{
    uid_t uid = geteuid();
    struct passwd *pw = getpwuid(uid);
    if (pw)
    {
        return pw->pw_name;
    }

    return "";
}

XP_STATUS pyDalInit()
{
    int i;
    char fpathw[XPSHELL_MAX_PIPE_NAME];
    char fpathr[XPSHELL_MAX_PIPE_NAME];
    struct passwd *pwd;
    pyDalDeInit();

    /* open, read, pipe */
    sprintf(&fpathr[0], "/tmp/npiper%s%d", xpGetUserName(), 0);
    /* Create pipe */
    if ((mkfifo(fpathr, 0777)) != 0)
    {
        printf("%s():%d: mkfifo failed with Error : %s\n", __FUNCTION__, __LINE__,
               strerror(errno));
        return XP_ERR_RESOURCE_NOT_AVAILABLE;
    }
    /* Change ownership */
    if ((pwd = getpwnam(USER_ADMIN)) != NULL)
    {
        if ((doChown(fpathr, USER_ROOT, USER_ADMIN)) != XP_NO_ERR)
        {
            printf("%s():%d: doChown failed with Error : %s\n", __FUNCTION__, __LINE__,
                   strerror(errno));
            return XP_ERR_INVALID_ARG;
        }
    }
    /* Change permission */
    if ((chmod(fpathr, 0775)) != 0)
    {
        printf("%s():%d: chmod failed wirh Error : %s\n", __FUNCTION__, __LINE__,
               strerror(errno));
        return XP_ERR_INVALID_ARG;
    }

    for (i=0; i<XPSHELL_MAX_SESSIONS; i++)
    {
        /* open, write, pipe */
        sprintf(&fpathw[0], "/tmp/npipew%s%d", xpGetUserName(), i);
        /* Create pipe */
        if ((mkfifo(fpathw, 0777)) != 0)
        {
            printf("%s():%d: mkfifo failed with Error : %s\n", __FUNCTION__, __LINE__,
                   strerror(errno));
            return XP_ERR_RESOURCE_NOT_AVAILABLE;
        }
        /* Change ownership */
        if ((pwd = getpwnam(USER_ADMIN)) != NULL)
        {
            if ((doChown(fpathw, USER_ROOT, USER_ADMIN)) != XP_NO_ERR)
            {
                printf("%s():%d: doChown failed with Error : %s\n", __FUNCTION__, __LINE__,
                       strerror(errno));
                return XP_ERR_INVALID_ARG;
            }
        }
        /* Change permission */
        if ((chmod(fpathw, 0775)) != 0)
        {
            printf("%s():%d: chmod failed with Error : %s\n", __FUNCTION__, __LINE__,
                   strerror(errno));
            return XP_ERR_INVALID_ARG;
        }
    }
    return (XP_NO_ERR);
}


XP_STATUS pyDalWrite(int sessionId, int dir)
{
    char fpathw[XPSHELL_MAX_PIPE_NAME], chr = 'r';
    int fd;
    char sstr[128];

    if (dir)
    {
        chr = 'w';
    }

    sprintf(&fpathw[0], "/tmp/npipe%c%s%d", chr, xpGetUserName(), sessionId);
    fd = open(fpathw, O_RDWR);
    if (fd == -1)
    {
        return (XP_ERR_FILE_OPEN);
    }

    sprintf(sstr, "%d\n", sessionId);
    if (write(fd, sstr, strlen(sstr)) < 0)
    {
        printf("Write failed with error: %s\n", strerror(errno));
    }

    /* to flush */
    close(fd);

    return (XP_NO_ERR);
}

XP_STATUS pyDalWriteSessionId(int sessionId, int dir, bool flag)
{
    char fpathw[XPSHELL_MAX_PIPE_NAME], chr = 'r';
    int fd;
    char sstr[128];
    int pipeno;
    memset(fpathw, 0, sizeof(fpathw));
    memset(sstr, 0, sizeof(sstr));

    if (dir)
    {
        chr = 'w';
    }

    pipeno = sessionId - ENTRY_POINT;
    snprintf(fpathw, sizeof(fpathw), "/tmp/npipe%c%s%d", chr, xpGetUserName(),
             pipeno);
    fd = open(fpathw, O_RDWR);
    if (fd == -1)
    {
        return (XP_ERR_FILE_OPEN);
    }

    if (!flag)
    {
        sessionId = NEGATIVE_FEEDBACK;
    }

    snprintf(sstr, sizeof(sstr), "%d\n", sessionId);
    if (write(fd, sstr, strlen(sstr)) < 0)
    {
        printf("Write failed with error: %s\n", strerror(errno));
    }
    /* to flush */
    close(fd);

    return (XP_NO_ERR);
}

XP_STATUS pyDalRead(int *sessionId, int dir)
{
    char fpathr[XPSHELL_MAX_PIPE_NAME], chr = 'r';
    int fd, len1;
    char value[XPSHELL_MAX_PIPE_NAME];

    if (dir)
    {
        chr = 'w';
    }
    sprintf(&fpathr[0], "/tmp/npipe%c%s%d", chr, xpGetUserName(), 0);
    fd = open(fpathr, O_RDWR);
    if (fd == -1)
    {
        return (XP_ERR_FILE_OPEN);
    }

    len1 = read(fd, &value, XPSHELL_MAX_PIPE_NAME);
    /* to flush */
    close(fd);

    sscanf(value, "%d\n", sessionId);

    if (len1 <= 0)
    {
        return (XP_ERR_FILE_READ);
    }
    else
    {
        // printf("Read something\n");
    }
    return (XP_NO_ERR);
}

static void * xpPyCliReqTask(void *arg)
{
    int currSessionId = *(int *)arg;
    char confFile[XPSHELL_MAX_PIPE_NAME];
    uint8_t sIdReg[MAX_CLIENT_ID + 1] = {0};    //Store the Session-Id Base on Client Request and Maintain the List
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    free(arg);
    while (1)
    {
        if (pyDalRead(&currSessionId, 0) != XP_NO_ERR)
        {
            // printf("Error in reading for session %d\n", i);
            sleep(4);
            continue;
        }

        /*Check and Available Session-Id Assign to Client*/
        if ((currSessionId >= (ENTRY_POINT + MIN_CLIENT_ID)) && (currSessionId <=
                                                                 (ENTRY_POINT + MAX_CLIENT_ID)))
        {
            if (sIdReg[currSessionId - ENTRY_POINT] == 0)
            {
                sIdReg[currSessionId - ENTRY_POINT] = 1;
                pyDalWriteSessionId(currSessionId, 1, 1);
            }
            else
            {
                pyDalWriteSessionId(currSessionId, 1, 0);
            }
            sleep(1);
            continue;
        }

        /*Free the assigned Session-Id when client exit the shell*/
        if ((currSessionId >= (EXIT_POINT + MIN_CLIENT_ID)) && (currSessionId <=
                                                                (EXIT_POINT + MAX_CLIENT_ID)))
        {
            sIdReg[currSessionId - EXIT_POINT] = 0;
            sleep(1);
            continue;
        }

        if (currSessionId >= XPSHELL_MAX_SESSIONS)
        {
            if (currSessionId == 9)
            {
                printf("Restoring to primary xpShell\n");
                // pyDalWrite(currSessionId, 1);
                shellRun = XP_SHELL_EXIT_5;
            }
            sleep(1);
            continue;
        }

        /* if primary shell is running then continue */
        if (shellRun != 0)
        {
            printf("Attempting to connect from secondary xpShells.\nPlease \'exit 2\' this primary session\n");
            /*if (0) // ((gShThread != 0) // && (threadCnt++ > 1))
            {
                printf("Killing this session to enable remote sessions\n");
                pthread_kill(gShThread, SIGUSR1);
                gShThread = 0;
                threadCnt = 0;
            }*/
            sleep(1);
            pyDalWrite(currSessionId, 1);
            continue;
        }
        sprintf(confFile, "%s%s%d", XP_CONFIG_TRANS_FILE, xpGetUserName(),
                currSessionId);
        xpPyInvoke(0, XP_SHELL_CLIENT, confFile);
    }
    printf("Exiting CLI session %d\n", currSessionId);
    return NULL;
}

XP_STATUS xpPyCliRequests(int sessionId)
{
    pthread_t thread1;
    int error;
    int *arg = (int *)malloc(sizeof(int));
    if (!arg)
    {
        printf("ERROR: Memory allocation for CLI Request argument failed.\n");
        free(arg);
        return XP_ERR_MEM_ALLOC_ERROR;
    }
    *arg = sessionId;

    xpPyInit();
    error = pthread_create(&thread1, NULL, xpPyCliReqTask, (void *)arg);
    if (error)
    {
        free(arg);
        return XP_ERR_INIT;
    }

    srvThread = thread1;
    srvThreadExists = true;

    // xpPyCliReqTask((void *)arg);
    return XP_NO_ERR;
}

bool xpsIsXpshellthreadActive(void)
{
    return srvThreadExists;
}

XP_STATUS xpsShellInit(void)
{
    return srvThreadExists ? XP_NO_ERR : xpPyCliRequests(0);
}

XP_STATUS xpsShellDeInit(void)
{
    XP_STATUS status = XP_NO_ERR;
    if (srvThreadExists)
    {
        status = (XP_STATUS)pthread_cancel(srvThread);
        if (status != XP_NO_ERR)
        {
            printf("Error: pthread_cancel Failed with Error Code: %d\n", status);
        }
        pthread_join(srvThread, NULL);
        pyDalDeInit();
        srvThreadExists = false;
    }
    return XP_NO_ERR;
}

XP_STATUS pyDalDeInit()
{
    int i;
    char fpathw[XPSHELL_MAX_PIPE_NAME];
    char fpathr[XPSHELL_MAX_PIPE_NAME];

    /* read pipe */
    sprintf(&fpathr[0], "/tmp/npiper%s%d", xpGetUserName(), 0);
    unlink(fpathr);

    /* remove the FIFO */
    for (i=0; i<XPSHELL_MAX_SESSIONS; i++)
    {
        sprintf(&fpathw[0], "/tmp/npipew%s%d", xpGetUserName(), i);
        unlink(fpathw);
    }
    return (XP_NO_ERR);
}
