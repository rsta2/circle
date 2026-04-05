/*
 * circle.ld
 */

ENTRY(_start)

SECTIONS
{
	.init : {
		*(.init)
	}

	.text : {
		*(.text*)

		_etext = .;
	}

	.rodata : {
		*(.rodata*)
	}

	.init_array : {
		__init_start = .;

		KEEP(*(.init_array*))

		__init_end = .;
	}

	.ARM.exidx : {
		__exidx_start = .;

		*(.ARM.exidx*)

		__exidx_end = .;
	}

	.eh_frame_hdr : {
		__eh_frame_hdr_start = .;

		*(.eh_frame_hdr) *(.eh_frame_hdr.*)

		__eh_frame_hdr_end = .;
	}

	.eh_frame : {
		__eh_frame_start = .;

		*(.eh_frame) *(.eh_frame.*)

		__eh_frame_end = .;
	}

	.data : {
		*(.data*)
	}

	.tdata : ALIGN(8) {
		__tdata_start = .;

		*(.tdata .tdata.*)

		__tdata_end = .;
	}

	.bss : {
		__bss_start = .;

		*(.bss*)
		*(COMMON)

		__bss_end = .;
	}

	.tbss (NOLOAD) : ALIGN(8) {
		__tbss_start = .;

		*(.tbss .tbss.*)

		__tbss_end = .;
	}

	_end = .;
}
