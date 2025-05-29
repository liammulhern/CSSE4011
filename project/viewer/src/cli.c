#include <zephyr/shell/shell.h>
#include <zephyr/settings/settings.h>
#include <string.h>
#include <errno.h>
#include <wifi.h>   /* declare extern wifi_ssid[], wifi_psk[] and wifi_connect() */

/* set SSID */
static int cmd_wifi_set_ssid(const struct shell *sh, size_t argc, char **argv)
{
    size_t len = strlen(argv[1]);

    if (len >= MAX_SSID_LEN) {
        shell_error(sh, "SSID too long (max %d)", MAX_SSID_LEN - 1);
        return -EINVAL;
    }

    /* copy into our variable */
    strcpy(wifi_ssid, argv[1]);

    /* save to flash */
    settings_save_one("wifi/ssid", wifi_ssid, len);

    shell_print(sh, "Saved SSID: %s", wifi_ssid);
    return 0;
}

/* set PSK */
static int cmd_wifi_set_psk(const struct shell *sh, size_t argc, char **argv)
{
    size_t len = strlen(argv[1]);

    if (len >= MAX_PSK_LEN) {
        shell_error(sh, "PSK too long (max %d)", MAX_PSK_LEN - 1);
        return -EINVAL;
    }

    strcpy(wifi_psk, argv[1]);
    settings_save_one("wifi/psk", wifi_psk, len);

    shell_print(sh, "Saved PSK");
    return 0;
}

/* show current credentials */
static int cmd_wifi_show(const struct shell *sh, size_t argc, char **argv)
{
    shell_print(sh, "Current SSID: %s", wifi_ssid);
    shell_print(sh, "Current PSK : %s", wifi_psk);
    return 0;
}

/* trigger a connect */
static int cmd_wifi_connect(const struct shell *sh, size_t argc, char **argv)
{
    shell_print(sh, "Connecting to %s …", wifi_ssid);
    wifi_connection_retry_reset();
    wifi_connect();
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(wifi_cmds,
    SHELL_CMD_ARG(ssid, NULL, "Set Wi-Fi SSID", cmd_wifi_set_ssid, 2, 0),
    SHELL_CMD_ARG(psk, NULL, "Set Wi-Fi PSK",  cmd_wifi_set_psk,  2, 0),
    SHELL_CMD(show, NULL, "Show Wi-Fi config", cmd_wifi_show),
    SHELL_CMD(connect, NULL, "Connect to Wi-Fi", cmd_wifi_connect),
    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(wifi, &wifi_cmds, "Wi-Fi commands", NULL);

