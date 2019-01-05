OUTPUT_FORMAT("elf32-microblaze", "elf32-microblaze", "elf32-microblaze")
SEARCH_DIR("./");
ENTRY(crtso);

SECTIONS
{
  .text : {
    __text_start = .;    
    KEEP(*(.crtso));
    *(.text);
    _etext = .;
    __etext = .;
    __text_end = .;
  }

  .data : {
    __data_start = .;
    *(.data);
    *(.sdata);
    *(.sdata2);
    *(.rodata);
    *(.rodata.*);
    _edata = .;
    __edata = .;
    __data_end = .;
  }
  
  .bss : {     
    __bss_start = .;
    *(.bss);
    *(.sbss);
    *(COMMON);
    end = .; 
    _end = .;
    __end = .;
    __bss_end = .;
  }
  
/* Stuff I don't want to know about. */

  /DISCARD/ : { 
    *(.note.GNU-stack); 
    *(.comment);
    *(.debug_*);
    *(.eh_frame);
    *(.got);    
    *(.data.rel);
    *(.data.rel.ro);
    *(.plt);
    *(.got.plt);
    *(.got2);
  }
  
}
  
