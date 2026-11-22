MEMORY
{
  /*
  # flash    (rx)  : ORIGIN = 0x00000000, LENGTH = 0x000A0000
  # sram     (rwx) : ORIGIN = 0x00800000, LENGTH = 0x00008000
  # dram     (rwx) : ORIGIN = 0x02000000, LENGTH = 0x01000000

  # Flash free zone: 000E_1000 - 000E_B000 -> A000 (40_960, ~40k) bytes
  # S-RAM safe zone: 0080_5e00 - 0080_7800 -> 1A00 (06_656,  ~6k) bytes
  # D-RAM safe zone: 0200_8900 - 0200_e300 -> 5A00 (23_040, ~23k) bytes
  */

  s_sjis   (r)   : ORIGIN = 0x000E0094, LENGTH = 0x00000F6C
  s_flash  (rx)  : ORIGIN = 0x000E1000, LENGTH = 0x0000A000
  /* Stock ROM padding after the programming image's last initialized word.
   * End before the next boot stub at 0xFB000. Read-only character data. */
  s_trad   (r)   : ORIGIN = 0x000F9100, LENGTH = 0x00001760
  s_wide   (rx)  : ORIGIN = 0x000FA860, LENGTH = 0x000007A0
  s_sram   (rwx) : ORIGIN = 0x00805e00, LENGTH = 0x00001A00
  s_dram   (rwx) : ORIGIN = 0x02008900, LENGTH = 0x00005A00
}

SECTIONS
{
    __cfw_start__ = LOADADDR(.text);

    .wide_title :
    {
        *remote_title_wide.o(.text* .rodata*)
        . = ALIGN(4);
    } > s_wide

    .text :
    {
        *(.text.init)
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
    } > s_flash

    .sjis_prefix :
    {
        *(.sjis_prefix)
        . = ALIGN(4);
    } > s_sjis

    .sjis_traditional :
    {
        *(.sjis_traditional)
        . = ALIGN(4);
    } > s_trad

    .data : AT (LOADADDR(.text) + SIZEOF(.text))
    {
        __data_start__ = .;
        *(.data*)
        . = ALIGN(4);  /* init() advances in 32-bit words */
        __data_end__ = .;
    } > s_sram

    __data_load__ = LOADADDR(.data);

    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);  /* init() advances in 32-bit words */
        __bss_end__ = .;
    } > s_sram
}

ASSERT(LOADADDR(.data) + SIZEOF(.data) <= ORIGIN(s_flash) + LENGTH(s_flash), "CFW initial data exceeds flash window");
