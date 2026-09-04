#include "init.h"
#include <debug.h>
#include <kernel.h>
#include <libcdvd-common.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <fileio.h>

#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif

#define DVRP_PRE_UPDATE_A 0x5646
#define DVRP_PRE_UPDATE_B 0x5647
#define DVRP_UPDATE_FIRMWARE 0x5676
#define DVRP_FLASH_WRITE_STATUS 0x5677

const char DEFAULT_FW_PATH[] = "xfrom:/DVRP_FIRMWARE.udm";

const uint8_t dvrpAuth[256] = {
  /* Place DVRP authentication data here */
};

int main(int argc, char *argv[]) {
  init_scr();
  scr_clear();
  scr_setCursor(0);

  scr_printf("\n\n\tPSX DVRP Flasher %s\n\tby pcm720\n\n", GIT_VERSION);

  scr_printf("\tInitializing modules\n");
  if (initModules() != 0) {
    scr_printf("\n\tERROR: Failed to init modules\n");
    goto fail;
  }
  fileXioInit();

  char firmwarePath[256] = {0};
  getcwd(firmwarePath, sizeof(firmwarePath) - 1);
  int len = strlen(firmwarePath);
  if (len > 0 && firmwarePath[len - 1] != '/')
    strcat(firmwarePath, "/");
  strcat(firmwarePath, "DVRP_FIRMWARE.udm");

  scr_printf("\tChecking for %s", firmwarePath);
  int fd = fileXioOpen(firmwarePath, FIO_O_RDONLY);
  if (fd < 0) {
    snprintf(firmwarePath, sizeof(firmwarePath), DEFAULT_FW_PATH);
    scr_printf(": not found\n\tChecking for %s", firmwarePath);
    fd = fileXioOpen(firmwarePath, FIO_O_RDONLY);
    if (fd < 0) {
      scr_printf("\n\n\n\tERROR: %s not found\n", firmwarePath);
      goto fail;
    }
  }
  fileXioClose(fd);

  uint8_t devctlbuf[0x10] = {0};

  scr_printf("\n\n\tStarting the update\n\n");
  scr_printf("\t1. Running pre-update commands\n");
  fd = fileXioDevctl("dvr:", DVRP_PRE_UPDATE_A, (void *)dvrpAuth, sizeof(dvrpAuth), devctlbuf, 0x2);
  if (fd < 0) {
    scr_printf("\n\tERROR: DVRP_PRE_UPDATE_1 failed: %d, status %02x\n", fd, *(uint16_t *)devctlbuf);
    goto fail;
  }
  fd = fileXioDevctl("dvr:", DVRP_PRE_UPDATE_B, devctlbuf, 0x2, 0, 0);
  if (fd < 0) {
    scr_printf("\n\tERROR: DVRP_PRE_UPDATE_2 failed: %d, status %02x\n", fd, *(uint16_t *)devctlbuf);
    goto fail;
  }

  scr_printf("\t2. Updating DVRP firmware\n");
  fd = fileXioDevctl("dvr_misc:", DVRP_UPDATE_FIRMWARE, firmwarePath, strlen(firmwarePath) + 1, 0, 0);
  if (fd < 0) {
    scr_printf("\n\tERROR: DVRP_UPDATE_FIRMWARE failed: %d\n", fd);
    goto fail;
  }

  uint32_t status = 0;
  scr_printf("\n");
  while (1) {
    sleep(1);
    fd = fileXioDevctl("dvr_misc:", DVRP_FLASH_WRITE_STATUS, 0, 0, &status, sizeof(status));
    if (fd < 0) {
      scr_printf("\n\n\tERROR: DVRP_FLASH_WRITE_STATUS failed: %d\n", fd);
      goto fail;
    }
    status &= 0xFFFF;
    if (status == 0xFFFF) {
      scr_printf("\n\n\tERROR: DVRP_FLASH_WRITE_STATUS: status %d\n", -1);
      goto fail;
    }

    scr_printf("\r\tChecking for DVRP flash write status: %ld", status);
    if (status == 2) {
      sceCdInit(SCECdINoD);
      if (0) {
        // Finish the update
        char inarg[1] = {0};
        char outarg[16] = {0};
        sceCdApplySCmd(0x3D, inarg, sizeof(inarg), outarg);
      }
      // Reset the DVRP
      scr_printf("\n\tResetting the DVRP after flashing\n");
      xdvrpReset(0);
      sceCdInit(SCECdEXIT);
      scr_printf("\n\n\tSuccess.\n\tThe system will reboot in 5 seconds.\n");
      sleep(5);
      ExecOSD(0, NULL);
      goto fail;
    }
  }

fail:
  sleep(10);
  ExecOSD(0, NULL);
}
