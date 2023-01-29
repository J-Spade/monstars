#include <linux/module.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "macros.h"

int main()
{
    int retval = 0;

    if (0 != syscall(SYS_init_module, (void *)c_KernelMod, sizeof(c_KernelMod), ""))
    {
        DEBUG_LOG("Failed to install MONSTARS_NF module!\n");
        retval = 1;
    }
    else
    {
        DEBUG_LOG("MONSTARS_NF installed!\n");
    }

    return retval;
}