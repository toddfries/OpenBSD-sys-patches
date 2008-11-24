#include <sys/cdefs.h>
__FBSDID("$FreeBSD: src/sys/xen/features.c,v 1.1 2008/08/15 21:20:44 kmacy Exp $");

#include <sys/types.h>
#include <machine/xen/xen-os.h>
#include <machine/xen/hypervisor.h>
#include <machine/xen/features.h>

uint8_t xen_features[XENFEAT_NR_SUBMAPS * 32] /* __read_mostly */;

void
setup_xen_features(void)
{
        xen_feature_info_t fi;
        int i, j;

        for (i = 0; i < XENFEAT_NR_SUBMAPS; i++) {
                fi.submap_idx = i;
                if (HYPERVISOR_xen_version(XENVER_get_features, &fi) < 0)
                        break;
                for (j = 0; j < 32; j++)
                        xen_features[i*32 + j] = !!(fi.submap & 1<<j);
        }
}
