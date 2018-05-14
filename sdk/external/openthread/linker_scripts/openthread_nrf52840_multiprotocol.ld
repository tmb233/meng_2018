/* Linker script to configure memory regions. */

SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x22000, LENGTH = 0xde000
  RAM (rwx) :  ORIGIN = 0x200026c0, LENGTH = 0x3d940
}

FLASH_PAGE_SIZE       = 4096;
FLASH_DATA_PAGES_USED = 4;

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
    __flash_data_end = (ORIGIN(FLASH) + LENGTH(FLASH));
    __flash_data_start = (__flash_data_end - (FLASH_PAGE_SIZE * FLASH_DATA_PAGES_USED));

    /* Assure that code does not overlap flash data area.*/
    ASSERT((__flash_data_start >= __etext + SIZEOF(.data)), "Error: Code overlaps flash data area.")
}

INCLUDE "nrf5x_common.ld"
