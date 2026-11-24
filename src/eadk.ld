SECTIONS {
  .rodata.eadk_app_name : {
    KEEP(*(.rodata.eadk_app_name))
  }
  .rodata.eadk_api_level : {
    KEEP(*(.rodata.eadk_api_level))
  }
  .rodata.eadk_app_icon : {
    KEEP(*(.rodata.eadk_app_icon))
  }
  .text : {
    *(.text)
    *(.text.*)
  }
  .rodata : {
    *(.rodata)
    *(.rodata.*)
  }
  .data : {
    *(.data)
    *(.data.*)
  }
  .bss : {
    *(.bss)
    *(.bss.*)
    *(COMMON)
  }
}
