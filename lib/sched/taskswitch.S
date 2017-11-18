/*
 * taskswitch.S
 *
 * Circle - A C++ bare metal environment for Raspberry Pi
 * Copyright (C) 2015-2017  R. Stange <rsta2@o2online.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

	.text

	.globl	TaskSwitch
TaskSwitch:					/* r0: pOldRegs, r1: pNewRegs */
	fmrx	r2, fpexc
	fmrx	r3, fpscr
	stmia	r0!, {r0, r2-r14}
	fstmiad	r0, {d0-d15}

	ldmia	r1!, {r0, r2-r14}
	fmxr	fpexc, r2
	fmxr	fpscr, r3
	fldmiad	r1, {d0-d15}

	bx	lr

/* End */
