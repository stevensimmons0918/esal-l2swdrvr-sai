#include <esal-vendor-api/esal_vendor_api.h>
#include <unistd.h>
#include "headers/esalSaiDef.h"
int main()
{

	DllInit();

    int vlan = 100;
    int rc = VendorCreateVlan(vlan);
    if(rc != ESAL_RC_OK)
    {
        printf("error");
    }

    uint16_t ports[] = {0, 1, 2, 5};

    rc = VendorAddPortsToVlan(vlan, 4, ports);

    if(rc != ESAL_RC_OK)
    {
        printf("error");
    }
    
    rc = VendorEnablePort(0);

    if(rc != ESAL_RC_OK)
    {
        printf("error");
    }

    rc = VendorEnablePort(1);

    if(rc != ESAL_RC_OK)
    {
        printf("error");
    }

    while (1)
    {
        usleep(2000000);
    }
	return 0;
}
