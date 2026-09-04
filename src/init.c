#include "init.h"
#include <debug.h>
#include <fcntl.h>
#include <iopcontrol.h>
#include <kernel.h>
#include <libcdvd.h>
#include <libpwroff.h>
#include <loadfile.h>
#include <ps2sdkapi.h>
#include <sbv_patches.h>
#include <sifrpc.h>
#include <stdint.h>
#include <string.h>
#define NEWLIB_PORT_AWARE
#include <fileio.h>

// Macros for loading embedded IOP modules
#define IRX_DEFINE(mod)                                                                                                                              \
  extern unsigned char mod##_irx[] __attribute__((aligned(16)));                                                                                     \
  extern uint32_t size_##mod##_irx

// Defines moduleList entry for embedded and external modules
#define INT_MODULE(mod) {#mod, NULL, mod##_irx, &size_##mod##_irx}

// Embedded IOP modules
IRX_DEFINE(iomanX);
IRX_DEFINE(fileXio);
IRX_DEFINE(sio2man);
IRX_DEFINE(mcman);
IRX_DEFINE(mcserv);
IRX_DEFINE(bdm);
IRX_DEFINE(bdmfs_fatfs);
IRX_DEFINE(usbd_mini);
IRX_DEFINE(usbmass_bd_mini);
IRX_DEFINE(ps2dev9);
IRX_DEFINE(xfromman);
IRX_DEFINE(extflash);
IRX_DEFINE(dvrdrv);
IRX_DEFINE(dvr);
IRX_DEFINE(dvrmisc);

typedef struct ModuleListEntry {
  char *name;         // Module name
  char *path;         // Module path for external modules
  unsigned char *irx; // Pointer to IRX module
  uint32_t *size;     // IRX size
} ModuleListEntry;

// List of modules to load
static ModuleListEntry moduleList[] = {
    INT_MODULE(iomanX),    INT_MODULE(fileXio), //
    INT_MODULE(sio2man),   INT_MODULE(mcman),
    INT_MODULE(mcserv),                                 //
    INT_MODULE(bdm),       INT_MODULE(bdmfs_fatfs),     //
    INT_MODULE(usbd_mini), INT_MODULE(usbmass_bd_mini), //
    INT_MODULE(ps2dev9),                                //
    INT_MODULE(extflash),  INT_MODULE(xfromman),        //
    INT_MODULE(dvrdrv),    INT_MODULE(dvr),
    INT_MODULE(dvrmisc),
};
#define MODULE_COUNT sizeof(moduleList) / sizeof(ModuleListEntry)

int switchPSXMode(int mode);
int xdvrpReset(uint8_t arg);

// Initializes IOP modules
int initModules() {
  int ret = 0;
  int iopret = 0;

  // Initialize the RPC manager and reboot the IOP with OSDSYS modules
  scr_printf("\t\tSwitching PSX into PSX mode\n");
  sceSifInitRpc(0);
  while (!SifIopReset("rom0:UDNL rom0:OSDCNF", 0)) {
  };
  while (!SifIopSync()) {
  };
  sceSifInitRpc(0);

  // Disable PS2 mode and reset DVRP before flashing
  sceCdInit(SCECdINoD);
  switchPSXMode(0);
  scr_printf("\t\tResetting the DVRP before flashing\n");
  xdvrpReset(0);
  sceCdInit(SCECdEXIT);

  // Reboot the IOP
  scr_printf("\t\tRebooting IOP\n");
  while (!SifIopReset("", 0)) {
  };
  while (!SifIopSync()) {
  };

  // Initialize the RPC manager
  sceSifInitRpc(0);

  // Apply patches required to load modules from EE RAM
  if ((ret = sbv_patch_enable_lmb()))
    return ret;
  if ((ret = sbv_patch_disable_prefix_check()))
    return ret;
  if ((ret = sbv_patch_fileio()))
    return ret;

  // Load modules
  for (int i = 0; i < MODULE_COUNT; i++) {
    ret = 0;
    iopret = 0;

    ret = SifExecModuleBuffer(moduleList[i].irx, *moduleList[i].size, 0, NULL, &iopret);
    if (ret >= 0)
      ret = 0;
    if (iopret == 1)
      ret = iopret;

    if (ret) {
      scr_printf("\n\t\tERROR: Failed to initialize module %s: %d\n", moduleList[i].name, ret);
      return ret;
    }
  }
  return 0;
}

#define SCMD_NOTICE_GAME_START 0x29
#define SCMD_XDVRP_RESET 0x33

// Switches between the PS2 and PSX modes
int switchPSXMode(int mode) {
  uint8_t in[4] = {mode};
  uint8_t out[16] = {};
  sceCdApplySCmd(SCMD_NOTICE_GAME_START, in, 4, out);
  return *(int *)out;
}

// Resets the DVRP
int xdvrpReset(uint8_t iplMode) {
  uint8_t in[1] = {iplMode};
  uint8_t out[16] = {};
  sceCdApplySCmd(SCMD_XDVRP_RESET, in, 1, out);
  return *(int *)out;
}
