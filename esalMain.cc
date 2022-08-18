#include <Python.h>
#include <unistd.h>
#include <stdio.h>

#include <esal-vendor-api/esal_vendor_api.h>
#include "headers/esalSaiDef.h"

void run_cli ()
{
    char cli_filename[] = "py/cli/cli.py";
    FILE* fp_cli;

    Py_Initialize();

    fp_cli = fopen(cli_filename, "r");
    PyRun_SimpleFile(fp_cli, cli_filename);

    Py_Finalize();
}

int main()
{

    DllInit();

    run_cli();

#if 0 // Undef for testing

    int vlan = 100;
    int rc = VendorCreateVlan(vlan);
    if(rc != ESAL_RC_OK)
    {
        printf("error");
    }

    uint16_t ports[] = {24, 25, 26, 5};

    rc = VendorAddPortsToVlan(vlan, 4, ports);

    if(rc != ESAL_RC_OK)
    {
        printf("error");
    }

    rc = VendorEnablePort(24);

    if(rc != ESAL_RC_OK)
    {
        printf("error");
    }

    rc = VendorEnablePort(25);

    if(rc != ESAL_RC_OK)
    {

    }
    uint16_t numPorts;
    uint16_t ports1[512];

    rc = VendorGetPortsInVlan(100,
                         &numPorts, ports1);

    if(rc != ESAL_RC_OK)
    {
        printf("error");
    }

    for(int i = 0; i < numPorts; i++ )
    {
        printf("port %d,", ports1[i]);
    }
#endif


    while (1)
    {
        usleep(2000000);
    }

	return 0;
}
