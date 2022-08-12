#include <esal-vendor-api/esal_vendor_api.h>
#include <unistd.h>
#include "headers/esalSaiDef.h"
#include "xpPyInc.h"
int main()
{

	DllInit();

    xpPyInit();
    xpPyInvoke(0, XP_SHELL_APP, NULL);
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
