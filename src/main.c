#include "init.h"
#include <debug.h>
#include <libcdvd-common.h>
#include <stdio.h>
#include <kernel.h>
#include <string.h>
#include <unistd.h>
#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <fileio.h>

#define DVRP_PRE_UPDATE_A 0x5646
#define DVRP_PRE_UPDATE_B 0x5647
#define DVRP_UPDATE_FIRMWARE 0x5676
#define DVRP_FLASH_WRITE_STATUS 0x5677

static char firmwarePath[] = "xfrom:/DVRP_FIRMWARE.udm";

const uint8_t dvrpAuth[256] = {
  /* Place DVRP authentication data here */
};

int main(int argc, char *argv[]) {
  init_scr();
  scr_clear();
  scr_setCursor(0);

  scr_printf("\n\n\tPSX DVRP Flasher 1.0\n\tby pcm720\n\n");

  scr_printf("\tInitializing modules\n");
  if (initModules() != 0) {
    scr_printf("\n\tERROR: Failed to init modules\n");
    goto fail;
  }
  fileXioInit();

  scr_printf("\tChecking for xfrom:/DVRP_FIRMWARE.udm\n\n");
  int fd = fileXioOpen(firmwarePath, FIO_O_RDONLY);
  if (fd < 0) {
    scr_printf("\n\tERROR: %s not found\n", firmwarePath);
    goto fail;
  }
  fileXioClose(fd);

  uint8_t devctlbuf[0x10] = {0};

  scr_printf("\tStarting the update\n\n");
  scr_printf("\t1. Running pre-update commands\n");
  fd = fileXioDevctl("dvr:", DVRP_PRE_UPDATE_A, (void*)dvrpAuth, sizeof(dvrpAuth), devctlbuf, 0x2);
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
      if (0) {
        char inarg[1] = {0};
        char outarg[16] = {0};
        sceCdInit(SCECdINoD);
        sceCdApplySCmd(0x3D, inarg, sizeof(inarg), outarg);
        sceCdInit(SCECdEXIT);
      }
      scr_printf("\n\n\tSuccess.\n\tHold the power button to turn off the console,\n\t then unplug the console from mains power for the update to apply\n");
      goto fail;
    }
  }

fail:
  sleep(5);
  __builtin_trap();
}
