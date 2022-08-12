// xpsAppShellInterface.c

/*******************************************************************************
* Copyright (c) 2021 Marvell. All rights reserved. The following file is       *
* subject to the limited use license agreement by and between Marvell and you, *
* your employer or other entity on behalf of whom you act. In the absence of   *
* such license agreement the following file is subject to Marvell’s standard   *
* Limited Use License Agreement.                                               *
********************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern int xpPyInvoke(int dev, int client, char *fileName);
//extern int xpAppWarmInit(int devType, int argc, char* argv[]);

#define MAX_NAME_SZ   (1024)

int startShell(char *configFileName)
{
    int status = 0;
    char verStr[MAX_NAME_SZ];

    while (1)
    {
        //xpAppWarmInit(1, 0, NULL);
        xpPyInvoke(0, 7, configFileName);
        if (configFileName == NULL)
        {
            configFileName = &verStr[0];
        }
        printf("Enter the command : ");
        /* Get the name, with size limit. */
        if (fgets(configFileName, MAX_NAME_SZ, stdin) == NULL)
        {
            continue;
        }

        if (strcmp(configFileName, "exit\n") == 0)
        {
            printf("Exiting xpShell\n");
            break;
        }
    }
    return status;
}

int shell_main(int argc, char *argv[])
{
    if (argc > 0)
    {
        startShell(argv[1]);
    }
    else
    {
        startShell(NULL);
    }
    return (0);
}

