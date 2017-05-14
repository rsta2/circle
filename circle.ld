/*
 * circle.ld
 */

SECTIONS
{
	. = 0x8000;
	
	.text : {
		*(.text*)

		_etext = .;
	}

	.rodata : {
		*(.rodata*)
	}

	.init_array : {
		__init_start = .;

		*(.init_array*)

		__init_end = .;
	}

	.data : {
		*(.data*)
	}

	.bss : {
		__bss_start = .;

		*(.bss*)

		_end = .;
	}
}
