/* avrOS linker script for AVR-Dx (AVR128DA28) on avr-gcc 14.2 / avr-libc.
 *
 * This is the stock avrxmega4_flmap.x device script (the FLMAP-aware default
 * that initializes NVMCTRL.CTRLB so the .rodata flash region is mapped into the
 * data address space at reset) with one addition: the avrOS resource tables
 * (CLI_CMDS, FSM_TABLE, ... UART_TABLE) are placed INSIDE the .rodata output
 * section. They must live in the FLMAP-mapped flash window so the runtime
 * table-walkers (gpioInit(), fsmDispatch(), the CLI, etc.) can reach them with
 * ordinary data loads. Left as orphan sections they land at low-flash VMAs that
 * alias SRAM in the data space, and the table walks read garbage.
 *
 * If you update the toolchain, re-derive this from the new
 * <toolchain>/lib/ldscripts/avrxmega4_flmap.x and re-apply the .rodata edit.
 */
OUTPUT_FORMAT("elf32-avr","elf32-avr","elf32-avr")
OUTPUT_ARCH(avr:104)
__TEXT_REGION_ORIGIN__ = DEFINED(__TEXT_REGION_ORIGIN__) ? __TEXT_REGION_ORIGIN__ : 0;
__TEXT_REGION_LENGTH__ = DEFINED(__TEXT_REGION_LENGTH__) ? __TEXT_REGION_LENGTH__ : 1024K;
__DATA_REGION_ORIGIN__ = DEFINED(__DATA_REGION_ORIGIN__) ? __DATA_REGION_ORIGIN__ : 0x802000;
__DATA_REGION_LENGTH__ = DEFINED(__DATA_REGION_LENGTH__) ? __DATA_REGION_LENGTH__ : 0xffa0;
__EEPROM_REGION_LENGTH__ = DEFINED(__EEPROM_REGION_LENGTH__) ? __EEPROM_REGION_LENGTH__ : 64K;
__FUSE_REGION_LENGTH__ = DEFINED(__FUSE_REGION_LENGTH__) ? __FUSE_REGION_LENGTH__ : 1K;
__LOCK_REGION_LENGTH__ = DEFINED(__LOCK_REGION_LENGTH__) ? __LOCK_REGION_LENGTH__ : 1K;
__SIGNATURE_REGION_LENGTH__ = DEFINED(__SIGNATURE_REGION_LENGTH__) ? __SIGNATURE_REGION_LENGTH__ : 1K;
__USER_SIGNATURE_REGION_LENGTH__ = DEFINED(__USER_SIGNATURE_REGION_LENGTH__) ? __USER_SIGNATURE_REGION_LENGTH__ : 1K;
__RODATA_VMA__ = 0xa00000;
__RODATA_LDS_OFFSET__ = DEFINED(__RODATA_LDS_OFFSET__) ? __RODATA_LDS_OFFSET__ : 0x8000;
__RODATA_REGION_LENGTH__ = DEFINED(__RODATA_REGION_LENGTH__) ? __RODATA_REGION_LENGTH__ : 32K;
__RODATA_ORIGIN__ = __RODATA_VMA__ + __RODATA_LDS_OFFSET__;
MEMORY
{
  text   (rx)   : ORIGIN = __TEXT_REGION_ORIGIN__, LENGTH = __TEXT_REGION_LENGTH__
  data   (rw!x) : ORIGIN = __DATA_REGION_ORIGIN__, LENGTH = __DATA_REGION_LENGTH__
  eeprom (rw!x) : ORIGIN = 0x810000, LENGTH = __EEPROM_REGION_LENGTH__
  fuse      (rw!x) : ORIGIN = 0x820000, LENGTH = __FUSE_REGION_LENGTH__
  lock      (rw!x) : ORIGIN = 0x830000, LENGTH = __LOCK_REGION_LENGTH__
  signature (rw!x) : ORIGIN = 0x840000, LENGTH = __SIGNATURE_REGION_LENGTH__
  user_signatures (rw!x) : ORIGIN = 0x850000, LENGTH = __USER_SIGNATURE_REGION_LENGTH__
  rodata (r!x) : ORIGIN = __RODATA_ORIGIN__, LENGTH = __RODATA_REGION_LENGTH__
}
SECTIONS
{
  /* Read-only sections, merged into text segment: */
  .hash          : { *(.hash)		}
  .dynsym        : { *(.dynsym)		}
  .dynstr        : { *(.dynstr)		}
  .gnu.version   : { *(.gnu.version)	}
  .gnu.version_d   : { *(.gnu.version_d)	}
  .gnu.version_r   : { *(.gnu.version_r)	}
  .rel.init      : { *(.rel.init)		}
  .rela.init     : { *(.rela.init)	}
  .rel.text      :
    {
      *(.rel.text)
      *(.rel.text.*)
      *(.rel.gnu.linkonce.t*)
    }
  .rela.text     :
    {
      *(.rela.text)
      *(.rela.text.*)
      *(.rela.gnu.linkonce.t*)
    }
  .rel.fini      : { *(.rel.fini)		}
  .rela.fini     : { *(.rela.fini)	}
  .rel.rodata    :
    {
      *(.rel.rodata)
      *(.rel.rodata.*)
      *(.rel.gnu.linkonce.r*)
    }
  .rela.rodata   :
    {
      *(.rela.rodata)
      *(.rela.rodata.*)
      *(.rela.gnu.linkonce.r*)
    }
  .rel.data      :
    {
      *(.rel.data)
      *(.rel.data.*)
      *(.rel.gnu.linkonce.d*)
    }
  .rela.data     :
    {
      *(.rela.data)
      *(.rela.data.*)
      *(.rela.gnu.linkonce.d*)
    }
  .rel.ctors     : { *(.rel.ctors)	}
  .rela.ctors    : { *(.rela.ctors)	}
  .rel.dtors     : { *(.rel.dtors)	}
  .rela.dtors    : { *(.rela.dtors)	}
  .rel.got       : { *(.rel.got)		}
  .rela.got      : { *(.rela.got)		}
  .rel.bss       : { *(.rel.bss)		}
  .rela.bss      : { *(.rela.bss)		}
  .rel.plt       : { *(.rel.plt)		}
  .rela.plt      : { *(.rela.plt)		}
  /* Internal text space or external memory.  */
  .text   :
  {
    *(.vectors)
    KEEP(*(.vectors))
    /* For data that needs to reside in the lower 64k of progmem.  */
    *(.progmem.gcc*)
    /* PR 13812: Placing the trampolines here gives a better chance
       that they will be in range of the code that uses them.  */
    . = ALIGN(2);
    __trampolines_start = . ;
    /* The jump trampolines for the 16-bit limited relocs will reside here.  */
    *(.trampolines)
    *(.trampolines*)
    __trampolines_end = . ;
    /* avr-libc expects these data to reside in lower 64K. */
    *libprintf_flt.a:*(.progmem.data)
    *libc.a:*(.progmem.data)
    *(.progmem.*)
    . = ALIGN(2);
    /* For code that needs to reside in the lower 128k progmem.  */
    *(.lowtext)
    *(.lowtext*)
     __ctors_start = . ;
     *(.ctors)
     __ctors_end = . ;
     __dtors_start = . ;
     *(.dtors)
     __dtors_end = . ;
    KEEP(SORT(*)(.ctors))
    KEEP(SORT(*)(.dtors))
    /* From this point on, we do not bother about whether the insns are
       below or above the 16 bits boundary.  */
    *(.init0)  /* Start here after reset.  */
    KEEP (*(.init0))
    *(.init1)
    KEEP (*(.init1))
    *(.init2)  /* Clear __zero_reg__, set up stack pointer.  */
    KEEP (*(.init2))
    *(.init3)
    KEEP (*(.init3))
    *(.init4)  /* Initialize data and BSS.  */
    KEEP (*(.init4))
    *(.init5)
    KEEP (*(.init5))
    *(.init6)  /* C++ constructors.  */
    KEEP (*(.init6))
    *(.init7)
    KEEP (*(.init7))
    *(.init8)
    KEEP (*(.init8))
    *(.init9)  /* Call main().  */
    KEEP (*(.init9))
    *(.text)
    . = ALIGN(2);
    *(.text.*)
    . = ALIGN(2);
    *(.fini9)  /* _exit() starts here.  */
    KEEP (*(.fini9))
    *(.fini8)
    KEEP (*(.fini8))
    *(.fini7)
    KEEP (*(.fini7))
    *(.fini6)  /* C++ destructors.  */
    KEEP (*(.fini6))
    *(.fini5)
    KEEP (*(.fini5))
    *(.fini4)
    KEEP (*(.fini4))
    *(.fini3)
    KEEP (*(.fini3))
    *(.fini2)
    KEEP (*(.fini2))
    *(.fini1)
    KEEP (*(.fini1))
    *(.fini0)  /* Infinite loop after program termination.  */
    KEEP (*(.fini0))
    /* For code that needs not to reside in the lower progmem.  */
    *(.hightext)
    *(.hightext*)
    *(.progmemx.*)
    . = ALIGN(2);
    /* For tablejump instruction arrays.  We do not relax
       JMP / CALL instructions within these sections.  */
    *(.jumptables)
    *(.jumptables*)
    _etext = . ;
  }  > text
  .data          :
  {
     PROVIDE (__data_start = .) ;
    *(.data)
     *(.data*)
    *(.gnu.linkonce.d*)
    . = ALIGN(2);
     _edata = . ;
     PROVIDE (__data_end = .) ;
  }  > data AT> text
  .bss  ADDR(.data) + SIZEOF (.data)   : AT (ADDR (.bss))
  {
     PROVIDE (__bss_start = .) ;
    *(.bss)
     *(.bss*)
     *(COMMON)
     PROVIDE (__bss_end = .) ;
  }  > data
   __data_load_start = LOADADDR(.data);
   __data_load_end = __data_load_start + SIZEOF(.data);
  /* Global data not cleared after reset.  */
  .noinit  ADDR(.bss) + SIZEOF (.bss)  :  AT (ADDR (.noinit))
  {
     PROVIDE (__noinit_start = .) ;
    *(.noinit .noinit.* .gnu.linkonce.n.*)
     PROVIDE (__noinit_end = .) ;
     _end = . ;
     PROVIDE (__heap_start = .) ;
  }  > data
__flmap_init_label = DEFINED(__flmap_init_start) ? __flmap_init_start : 0 ;
/* User can specify position of .rodata in flash (LMA) by supplying
   __RODATA_FLASH_START__ or __flmap, where the former takes precedence. */
__RODATA_FLASH_START__ = DEFINED(__RODATA_FLASH_START__)
   ? __RODATA_FLASH_START__
   : DEFINED(__flmap) ? __flmap * 32K : 96K;
ASSERT (__RODATA_FLASH_START__ % 32K == 0, "__RODATA_FLASH_START__ must be a multiple of 32 KiB")
__flmap = 0x3 & (__RODATA_FLASH_START__ >> 15);
__RODATA_FLASH_START__ = __flmap << 15;
__rodata_load_start = MAX (__data_load_end, __RODATA_FLASH_START__);
__rodata_start = __RODATA_ORIGIN__ + __rodata_load_start - __RODATA_FLASH_START__;
  .rodata  __rodata_start   :  AT (__rodata_load_start)
  {
    /* --- avrOS: window spans compiler rodata + the OS resource tables --- */
    __start_text_window = . ;
    *(.rodata)
     *(.rodata*)
     *(.gnu.linkonce.r*)
    /* End of compiler-generated read-only data / start of the OS tables.
       mem.h uses these boundaries to report rodata vs. os-table usage. */
    __stop_rodata = . ;
    /* --- avrOS resource tables (must be in the FLMAP-mapped window) --- */
    . = ALIGN(2);
    __start_CLI_CMDS = . ;
    KEEP(*(CLI_CMDS))
    __stop_CLI_CMDS = . ;
    . = ALIGN(2);
    __start_FSM_TABLE = . ;
    KEEP(*(FSM_TABLE))
    __stop_FSM_TABLE = . ;
    . = ALIGN(2);
    __start_QUE_TABLE = . ;
    KEEP(*(QUE_TABLE))
    __stop_QUE_TABLE = . ;
    . = ALIGN(2);
    __start_TMR_TABLE = . ;
    KEEP(*(TMR_TABLE))
    __stop_TMR_TABLE = . ;
    . = ALIGN(2);
    __start_TEST_TABLE = . ;
    KEEP(*(TEST_TABLE))
    __stop_TEST_TABLE = . ;
    . = ALIGN(2);
    __start_EVNT_TABLE = . ;
    KEEP(*(EVNT_TABLE))
    __stop_EVNT_TABLE = . ;
    . = ALIGN(2);
    __start_GPIO_TABLE = . ;
    KEEP(*(GPIO_TABLE))
    __stop_GPIO_TABLE = . ;
    . = ALIGN(2);
    __start_UART_TABLE = . ;
    KEEP(*(UART_TABLE))
    __stop_UART_TABLE = . ;
    . = ALIGN(2);
    __stop_text_window = . ;
     __rodata_end = ABSOLUTE(.) ;
  }  > rodata
 __rodata_load_end = __rodata_load_start + __rodata_end - __rodata_start;
__do_init_flmap = 1;
__flmap_value = __flmap << (DEFINED(__flmap_bpos) ? __flmap_bpos : 4);
__flmap_value_with_lock =
   __flmap_value | (DEFINED(__flmap_lock) && DEFINED(__flmap_lock_mask)
                    ? (__flmap_lock && __do_init_flmap ? __flmap_lock_mask : 0)
                    : 0);
  .eeprom  :
  {
    /* See .data above...  */
    KEEP(*(.eeprom*))
     __eeprom_end = . ;
  }  > eeprom
  .fuse  :
  {
    KEEP(*(.fuse))
    KEEP(*(.lfuse))
    KEEP(*(.hfuse))
    KEEP(*(.efuse))
  }  > fuse
  .lock  :
  {
    KEEP(*(.lock*))
  }  > lock
  .signature  :
  {
    KEEP(*(.signature*))
  }  > signature
  /* Stabs debugging sections.  */
  .stab          0 : { *(.stab) }
  .stabstr       0 : { *(.stabstr) }
  .stab.excl     0 : { *(.stab.excl) }
  .stab.exclstr  0 : { *(.stab.exclstr) }
  .stab.index    0 : { *(.stab.index) }
  .stab.indexstr 0 : { *(.stab.indexstr) }
  .comment 0 (INFO) : { *(.comment); LINKER_VERSION; }
  .gnu.build.attributes : { *(.gnu.build.attributes .gnu.build.attributes.*) }
  .note.gnu.build-id   : { *(.note.gnu.build-id) }
  /* DWARF debug sections.
     Symbols in the DWARF debugging sections are relative to the beginning
     of the section so we begin them at 0.  */
  /* DWARF 1.  */
  .debug          0 : { *(.debug) }
  .line           0 : { *(.line) }
  /* GNU DWARF 1 extensions.  */
  .debug_srcinfo  0 : { *(.debug_srcinfo) }
  .debug_sfnames  0 : { *(.debug_sfnames) }
  /* DWARF 1.1 and DWARF 2.  */
  .debug_aranges  0 : { *(.debug_aranges) }
  .debug_pubnames 0 : { *(.debug_pubnames) }
  /* DWARF 2.  */
  .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
  .debug_abbrev   0 : { *(.debug_abbrev) }
  .debug_line     0 : { *(.debug_line .debug_line.* .debug_line_end) }
  .debug_frame    0 : { *(.debug_frame) }
  .debug_str      0 : { *(.debug_str) }
  .debug_loc      0 : { *(.debug_loc) }
  .debug_macinfo  0 : { *(.debug_macinfo) }
  /* SGI/MIPS DWARF 2 extensions.  */
  .debug_weaknames 0 : { *(.debug_weaknames) }
  .debug_funcnames 0 : { *(.debug_funcnames) }
  .debug_typenames 0 : { *(.debug_typenames) }
  .debug_varnames  0 : { *(.debug_varnames) }
  /* DWARF 3.  */
  .debug_pubtypes 0 : { *(.debug_pubtypes) }
  .debug_ranges   0 : { *(.debug_ranges) }
  /* DWARF 5.  */
  .debug_addr     0 : { *(.debug_addr) }
  .debug_line_str 0 : { *(.debug_line_str) }
  .debug_loclists 0 : { *(.debug_loclists) }
  .debug_macro    0 : { *(.debug_macro) }
  .debug_names    0 : { *(.debug_names) }
  .debug_rnglists 0 : { *(.debug_rnglists) }
  .debug_str_offsets 0 : { *(.debug_str_offsets) }
  .debug_sup      0 : { *(.debug_sup) }
}
