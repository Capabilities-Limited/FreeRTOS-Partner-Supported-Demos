OUTPUT_ARCH( "riscv" )
ENTRY( _start )

MEMORY
{
	/* Fake ROM area */
	rom (rxa) : ORIGIN = 0x80000000, LENGTH = 512K
	ram (wxa) : ORIGIN = 0x80080000, LENGTH = 128M
}

SECTIONS
{
	.init :
	{
		_text = .;
		KEEP (*(SORT_NONE(.init)))
	} >rom
	
	.text :
	{
		*(.text.unlikely .text.unlikely.*)
		*(.text.startup .text.startup.*)
		*(.text .text.*)
		*(.gnu.linkonce.t.*)
	} >rom
	
	.fini :
	{
		KEEP (*(SORT_NONE(.fini)))
		_etext = .;
	} >rom

	.rodata.align :
	{
		. = ALIGN(8);
		_rodata = .;
	} >rom

	.rodata.start :
	{
		_rodata_lma = LOADADDR(.rodata.start);
	} >rom

	.rodata :
	{
		*(.rdata)
		*(.rodata .rodata.*)
		*(.gnu.linkonce.r.*)

		. = ALIGN(8);
		_erodata = .;
	} >rom

	.data.align :
	{
		. = ALIGN(8);
		_data = .;
	} >rom

	.data.start :
	{
		_data_lma1 = LOADADDR(.data.start);
	} >rom

	_data_lma = ABSOLUTE(.);
	.data :
	{
		_data = .;
		*(.data .data.*)
		*(.gnu.linkonce.d.*)
		. = ALIGN(8);
		PROVIDE( __global_pointer$ = . + 0x800 );
		*(.sdata .sdata.*)
		*(.sdata2 .sdata2.*)
		*(.gnu.linkonce.s.*)
		. = ALIGN(8);
		*(.srodata.cst16)
		*(.srodata.cst8)
		*(.srodata.cst4)
		*(.srodata.cst2)
		*(.srodata .srodata.*)

		. = ALIGN(8);
		_edata = .;
	} >ram AT>rom

	.bss.align :
	{
		. = ALIGN(8);
		_bss = .;
	} >ram

	.bss.start :
	{
		_bss_lma = LOADADDR(.bss.start);
	} >ram

	.bss :
	{
		*(.sbss*)
		*(.gnu.linkonce.sb.*)
		*(.bss .bss.*)
		*(.gnu.linkonce.b.*)
		*(COMMON)

		. = ALIGN(8);
		_ebss = .;
	} >ram

	. = ALIGN(8);
	_end = .;

	.stack :
	{
		. = ALIGN(8);
		. += __stack_size;
		_stack_top = .;
	} >ram
}
