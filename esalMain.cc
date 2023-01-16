#include <Python.h>
#include <unistd.h>
#include <stdio.h>

#include <esal_vendor_api/esal_vendor_api.h>
#include <esal_warmboot_api/esal_warmboot_api.h>
#include "headers/esalSaiDef.h"

extern "C" bool run_acl_samples(void);

// #define REG_RESTORE

#ifdef REG_RESTORE
#include "headers/esalCpssDefs.h"
typedef struct
{
    GT_U32  regAddr;
    GT_U32  value;
} regsToRestore_t;

void restoreRegisters() {
    regsToRestore_t regsToRestore[] =
    #include "registers.h"

    for (auto regval : regsToRestore) {
        prvCpssDrvHwPpWriteRegister(0, regval.regAddr, regval.value);
    }
}

#endif

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

#if 1 // Undef for shell

    if (DllInit() == ESAL_RC_OK) {
        // vendor_vlan_translation_t trans;
        // trans.newVlan = 110;
        // trans.oldVlan = 100;
        // //VendorSetIngressVlanTranslation(28, trans);
        VendorSetPortNniMode(28, VENDOR_NNI_MODE_UNI);

        vendor_vlan_translation_t vv_tr;

        vv_tr.oldVlan = 2050;
        vv_tr.newVlan = 2060;
        VendorSetIngressVlanTranslation(10, vv_tr);
        vv_tr.oldVlan = 2053;
        vv_tr.newVlan = 2040;
        VendorSetIngressVlanTranslation(10, vv_tr);
        vv_tr.oldVlan = 3950;
        vv_tr.newVlan = 3900;
        VendorSetIngressVlanTranslation(10, vv_tr);
        vv_tr.oldVlan = 3951;
        vv_tr.newVlan = 3948;
        VendorSetIngressVlanTranslation(10, vv_tr);

        vv_tr.oldVlan = 2060;
        vv_tr.newVlan = 2050;
        VendorSetIngressVlanTranslation(10, vv_tr);
        vv_tr.oldVlan = 2040;
        vv_tr.newVlan = 2053;
        VendorSetIngressVlanTranslation(10, vv_tr);
        vv_tr.oldVlan = 3900;
        vv_tr.newVlan = 3950;
        VendorSetIngressVlanTranslation(10, vv_tr);
        vv_tr.oldVlan = 3948;
        vv_tr.newVlan = 3951;
        VendorSetIngressVlanTranslation(10, vv_tr);

        vv_tr.oldVlan = 2050;
        vv_tr.newVlan = 2061;
        VendorSetIngressVlanTranslation(13, vv_tr);
        vv_tr.oldVlan = 2053;
        vv_tr.newVlan = 2041;
        VendorSetIngressVlanTranslation(13, vv_tr);
        vv_tr.oldVlan = 3950;
        vv_tr.newVlan = 3901;
        VendorSetIngressVlanTranslation(13, vv_tr);
        vv_tr.oldVlan = 3951;
        vv_tr.newVlan = 3949;
        VendorSetIngressVlanTranslation(13, vv_tr);

        vv_tr.oldVlan = 2061;
        vv_tr.newVlan = 2050;
        VendorSetIngressVlanTranslation(13, vv_tr);

#ifdef REG_RESTORE
        restoreRegisters();
#endif

        if (ESAL_WARM) {
            createFolderIfNotExist(BACKUP_FOLDER);
            esalWarmBootHandler();
        }

        run_cli();
    }

#endif

#if 0 // Undef for testing

    int rc = DllInit();
    if (rc != ESAL_RC_OK) {
        printf("error");
    }

    int vlan = 100;
    rc = VendorCreateVlan(vlan);
    if(rc != ESAL_RC_OK)
    {
        printf("error");
    }

    uint16_t ports[] = {28, 29, 30, 5};

    rc = VendorAddPortsToVlan(vlan, 4, ports);

    if(rc != ESAL_RC_OK)
    {
        printf("error");
    }

    rc = VendorEnablePort(28);

    if(rc != ESAL_RC_OK)
    {
        printf("error");
    }

    rc = VendorEnablePort(29);

    if(rc != ESAL_RC_OK)
    {
        printf("error");
    }

    rc = VendorEnablePort(30);

    if(rc != ESAL_RC_OK)
    {
        printf("error");
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

    run_acl_samples();

    while (1)
    {
        usleep(2000000);
    }

#endif

	return 0;
}
