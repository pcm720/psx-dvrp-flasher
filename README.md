# PSX DVRP Flasher

A utility for flashing DVRP firmware on PSX DESR systems.

## Usage

1. Place your DVRP firmware file named `DVRP_FIRMWARE.udm` into `xfrom:/DVRP_FIRMWARE.udm`, or onto a USB drive or memory card (in the same directory as `dvrp_flasher.elf`)
2. Run `dvrp_flasher.elf`

3. The utility will:
   - Switch the PSX into PSX mode and initialize all required IOP modules
   - Verify the presence of the firmware file
   - Flash the firmware to the DVRP hardware

4. Upon successful completion, the utility will display:
```
	Success.
	Hold the power button to turn off the console,
	 then unplug the console from mains power for the update to take effect
```

5. Follow the instructions to power cycle the console for the update to take effect.

## Credits
- uyjulian for documenting the update process [here](https://www.psdevwiki.com/ps2/DVRP)
- MonkeyBoyJoey for taking the risk and being the first to test the utility
