/* Implementation of profiling support.
   Copyright (C) 2008-2020 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library.  If not, see
   <https://www.gnu.org/licenses/>.  */

	.text

#if AARCH == 32

/* Use an assembly stub with a special ABI.  The calling lr has been
   pushed to the stack (which will be misaligned).  We should preserve
   all registers except ip and pop a word off the stack.

   NOTE: This assumes mcount_internal does not clobber any non-core
   (coprocessor) registers.  Currently this is true, but may require
   additional attention in the future.

   The calling sequence looks something like:
func:
   push {lr}
   bl __gnu_mcount_nc
   <function body>
*/
	.globl	__gnu_mcount_nc
__gnu_mcount_nc:
	push	{r0-r3, lr}
	bic	r1, lr, #1			/* selfpc */
	ldr	r0, [sp, #20]			/* frompc has been pushed to stack */
	bl	__mcount_internal
	pop	{r0-r3, ip, lr}
	bx	ip

#else

	.globl	_mcount
_mcount:					/* frompc is in x0 on entry */
	mov	x1, x30				/* selfpc */
	b	__mcount_internal

#endif

/* End */
