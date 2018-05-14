/* Linker script to configure memory regions. */

SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x1f000, LENGTH = 0xD5000
  RAM (rwx) :  ORIGIN = 0x20005000, LENGTH = 0x3B000

  /** Location of bootloader setting in flash. */
  BOOTLOADER_SETTINGS (rw) : ORIGIN = 0x000FF000, LENGTH = 0x1000

  /** Location of OpenThread settings in flash. */
  OPENTHREAD_SETTINGS (rw) : ORIGIN = 0x000F4000, LENGTH = 0x4000
}

SECTIONS
{
  /* Place the bootloader settings page in flash. */
  .bootloaderSettings(NOLOAD) :
  {

  } > BOOTLOADER_SETTINGS
}

SECTIONS
{
  nrf_radio_buffer (NOLOAD):
  {
      __nrf_radio_buffer_start__ = .;
      *(nrf_radio_buffer*)
      __nrf_radio_buffer_end__ = .;
  } > RAM
} INSERT BEFORE .data;

SECTIONS
{
  .fs_data :
  {
    PROVIDE(__start_fs_data = .);
    KEEP(*(.fs_data))
    PROVIDE(__stop_fs_data = .);
  } > RAM
} INSERT AFTER .data;

SECTIONS
{
  .pwr_mgmt_data :
  {
    PROVIDE(__start_pwr_mgmt_data = .);
    KEEP(*(SORT(.pwr_mgmt_data*)))
    PROVIDE(__stop_pwr_mgmt_data = .);
  } > FLASH
} INSERT AFTER .text

SECTIONS
{
    __flash_data_end = (ORIGIN(OPENTHREAD_SETTINGS) + LENGTH(OPENTHREAD_SETTINGS));
    __flash_data_start = ORIGIN(OPENTHREAD_SETTINGS);
}

INCLUDE "nrf5x_common.ld"
