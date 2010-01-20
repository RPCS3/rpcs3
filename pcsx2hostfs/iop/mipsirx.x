/* Link script for PlayStation 2 IRXs
 * Written by Douglas C. Knight <fsdck@uaf.edu>
 */
OUTPUT_FORMAT("elf32-littlemips")
 SEARCH_DIR(/home/karmix/local/iop/lib);
ENTRY(_start)
SECTIONS
{
  /* This is the .iopmod section for the IRX, it contains
     information that the IOP uses when loading the IRX.
     This section is placed in its own segment.  */
  .iopmod : {
    /* The linker will replace this first LONG with a pointer
       to _irx_id if the symbol has been defined.  */
    LONG (0xffffffff) ;
    LONG (_start) ;
    LONG (_gp) ;
    LONG (_text_size) ;
    LONG (_data_size) ;
    LONG (_bss_size) ;
    /* The linker will put a SHORT here with the version of
       the IRX (or zero if there is no version).  */
    /* The linker will put a null terminated string here
       containing the name of the IRX (or an empty string if
       the name is not known).  */
  }
  . = 0x0 ;
  _ftext = . ;
  .text : {
    CREATE_OBJECT_SYMBOLS
    * ( .text )
    * ( .text.* )
    * ( .init )
    * ( .fini )
  } = 0
  _etext  =  . ;
  . = . ;
  _fdata = . ;
  .rodata : {
    * ( .rdata )
    * ( .rodata )
    * ( .rodata1 )
    * ( .rodata.* )
  } = 0
  .data : {
    * ( .data )
    * ( .data1 )
    * ( .data.* )
    CONSTRUCTORS
  }
  . = ALIGN(16) ;
  _gp = . + 0x8000 ;
  .sdata : {
    * ( .lit8 )
    * ( .lit4 )
    * ( .sdata )
    * ( .sdata.* )
  }
  _edata = . ;
  . = ALIGN(4) ;
  _fbss = . ;
  .sbss : {
    * ( .sbss )
    * ( .scommon )
  }
  _bss_start = . ;
  .bss : {
    * ( .bss )
    * ( COMMON )
    . = ALIGN(4) ;
  }
  _end = . ;
  _text_size = _etext - _ftext ;
  _data_size = _edata - _fdata ;
  _bss_size = _end - _fbss ;
  /* This is the stuff that we don't want to be put in an IRX.  */
  /DISCARD/ : {
	* ( .reginfo )
  }
}
