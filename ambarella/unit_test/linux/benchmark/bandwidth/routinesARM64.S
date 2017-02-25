
# ============================================================================
#  bandwidth 0.23, a benchmark to estimate memory transfer bandwidth.
#  ARM assembly module.
#  Copyright (C) 2010 by Zack T Smith.
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#
#  The author may be reached at fbui@comcast.net.
# =============================================================================

#--------------
# Version 0.7
#--------------

#include "config.h"

.arch armv8-a

#ifdef __thumb2__
.syntax unified
.code 16
#endif

.section code

.text
.align 2

.global Writer
.global RandomWriter

.global Reader
.global RandomReader

.global	RegisterToRegister
.global	StackReader
.global	StackWriter

#-----------------------------------------------------------------------------
# Name: 	Writer
# Purpose:	Performs sequential write into memory, as fast as possible.
# Params:
#	x0 = address
#	x1 = length, multiple of 256
#	x2 = loop
# 	x3 = value to write
#-----------------------------------------------------------------------------
Writer:
	stp	x29, x30, [sp, #-16]!

	bic	x1, x1, #0x7f
	mov	x4, x0
	mov	x5, x1

	mov	x6, x3

# x4 = temp address
# x5 = temp length

.L0:
	mov	x0, x4
	mov	x1, x5

.L1:
	stp x3, x6, [x0]
	stp x3, x6, [x0, #16]
	stp x3, x6, [x0, #32]
	stp x3, x6, [x0, #48]
	stp x3, x6, [x0, #64]
	stp x3, x6, [x0, #80]
	stp x3, x6, [x0, #96]
	stp x3, x6, [x0, #112]
	stp x3, x6, [x0, #128]
	stp x3, x6, [x0, #144]
	stp x3, x6, [x0, #160]
	stp x3, x6, [x0, #176]
	stp x3, x6, [x0, #192]
	stp x3, x6, [x0, #208]
	stp x3, x6, [x0, #224]
	stp x3, x6, [x0, #240]
	add x0, x0, #256

	sub	x1, x1, #256
	cbnz	x1, .L1

	sub	x2, x2, #1
	cbnz	x2, .L0

# return.
	ldp x29, x30, [sp], #16
	ret

#-----------------------------------------------------------------------------
# Name: 	Reader
# Purpose:	Performs sequential reads from memory, as fast as possible.
# Params:
#	x0 = address
#	x1 = length, multiple of 256
#	x2 = loop
#-----------------------------------------------------------------------------
Reader:
	stp	x29, x30, [sp, #-16]!
	stp	x20, x21, [sp, #-16]!
	stp	x18, x19, [sp, #-16]!
	stp	x16, x17, [sp, #-16]!
	stp	x14, x15, [sp, #-16]!
	stp	x12, x13, [sp, #-16]!
	stp	x10, x11, [sp, #-16]!
	stp	x8, x9, [sp, #-16]!

	bic	x1, x1, #0x7f
	mov	x4, x0
	mov	x5, x1

# x4 = temp address
# x5 = temp length

.L2:
	mov	x0, x4
	mov	x1, x5

.L3:
	ldp x3, x6, [x0]
	ldp x7, x8, [x0, #16]
	ldp x9, x10, [x0, #32]
	ldp x11, x12, [x0, #48]
	ldp x13, x14, [x0, #64]
	ldp x15, x16, [x0, #80]
	ldp x17, x18, [x0, #96]
	ldp x19, x20, [x0, #112]
	ldp x21, x6, [x0, #128]
	ldp x7, x8, [x0, #144]
	ldp x9, x10, [x0, #160]
	ldp x11, x12, [x0, #176]
	ldp x13, x14, [x0, #192]
	ldp x15, x16, [x0, #208]
	ldp x17, x18, [x0, #224]
	ldp x19, x20, [x0, #240]
	add x0, x0, #256

	sub	x1, x1, #256
	cbnz	x1, .L3

	sub	x2, x2, #1
	cbnz	x2, .L2

# return.
	ldp x8, x9, [sp], #16
	ldp x10, x11, [sp], #16
	ldp x12, x13, [sp], #16
	ldp x14, x15, [sp], #16
	ldp x16, x17, [sp], #16
	ldp x18, x19, [sp], #16
	ldp x20, x21, [sp], #16
	ldp x29, x30, [sp], #16
	ret

#-----------------------------------------------------------------------------
# Name: 	RandomWriter
# Purpose:	Performs random write into memory, as fast as possible.
# Params:
# 	x0 = pointer to array of chunk pointers
# 	x1 = # of 256-byte chunks
# 	x2 = # loops to do
# 	x3 = value to write
#-----------------------------------------------------------------------------
RandomWriter:
	stp	x29, x30, [sp, #-16]!

# x4 = temp
# x5 = temp

.L4:
	mov	x5, #0

.L5:
# Get pointer to chunk in memory. Note, 64-bit pointers.
	ldr	x4, [x0, x5, LSL #3]

# Does 32 transfers, 8 bytes each = 256 bytes total.

	str	x3, [x4, #160]
	str	x3, [x4, #232]
	str	x3, [x4, #224]
	str	x3, [x4, #96]
	str	x3, [x4, #168]
	str	x3, [x4, #80]
	str	x3, [x4, #104]
	str	x3, [x4, #248]
	str	x3, [x4, #8]
	str	x3, [x4, #136]
	str	x3, [x4, #112]
	str	x3, [x4, #200]
	str	x3, [x4, #128]
	str	x3, [x4, #152]
	str	x3, [x4, #216]
	str	x3, [x4]
	str	x3, [x4, #88]
	str	x3, [x4, #144]
	str	x3, [x4, #208]
	str	x3, [x4, #184]
	str	x3, [x4, #48]
	str	x3, [x4, #64]
	str	x3, [x4, #240]
	str	x3, [x4, #24]
	str	x3, [x4, #72]
	str	x3, [x4, #32]
	str	x3, [x4, #56]
	str	x3, [x4, #16]
	str	x3, [x4, #40]
	str	x3, [x4, #176]
	str	x3, [x4, #120]
	str	x3, [x4, #192]

	add	x5, x5, #1
	cmp	x5, x1
	bne	.L5

	sub	x2, x2, #1
	cbnz	x2, .L4

# return.
	ldp x29, x30, [sp], #16
	ret

#-----------------------------------------------------------------------------
# Name: 	RandomReader
# Purpose:	Performs random reads from memory, as fast as possible.
# Params:
# 	x0 = pointer to array of chunk pointers
# 	x1 = # of 256-byte chunks
# 	x2 = # loops to do
#-----------------------------------------------------------------------------
RandomReader:
	stp	x29, x30, [sp, #-16]!

# x4 = temp
# x5 = temp

.L6:
	mov	x5, #0

.L7:
# Get pointer to chunk in memory. Note, 64-bit pointers.
	ldr	x4, [x0, x5, LSL #3]

# Does 32 transfers, 8 bytes each = 256 bytes total.

	ldr	x3, [x4, #160]
	ldr	x3, [x4, #232]
	ldr	x3, [x4, #224]
	ldr	x3, [x4, #96]
	ldr	x3, [x4, #168]
	ldr	x3, [x4, #80]
	ldr	x3, [x4, #104]
	ldr	x3, [x4, #248]
	ldr	x3, [x4, #8]
	ldr	x3, [x4, #136]
	ldr	x3, [x4, #112]
	ldr	x3, [x4, #200]
	ldr	x3, [x4, #128]
	ldr	x3, [x4, #152]
	ldr	x3, [x4, #216]
	ldr	x3, [x4]
	ldr	x3, [x4, #88]
	ldr	x3, [x4, #144]
	ldr	x3, [x4, #208]
	ldr	x3, [x4, #184]
	ldr	x3, [x4, #48]
	ldr	x3, [x4, #64]
	ldr	x3, [x4, #240]
	ldr	x3, [x4, #24]
	ldr	x3, [x4, #72]
	ldr	x3, [x4, #32]
	ldr	x3, [x4, #56]
	ldr	x3, [x4, #16]
	ldr	x3, [x4, #40]
	ldr	x3, [x4, #176]
	ldr	x3, [x4, #120]
	ldr	x3, [x4, #192]

	add	x5, x5, #1
	cmp	x5, x1
	bne	.L7

	sub	x2, x2, #1
	cbnz	x2, .L6

# return.
	ldp x29, x30, [sp], #16
	ret

#-----------------------------------------------------------------------------
# Name: 	RegisterToRegister
# Purpose:	Performs register-to-register transfers.
# Params:
#	x0 = count
#-----------------------------------------------------------------------------
RegisterToRegister:
	stp	x29, x30, [sp, #-16]!

# x1 = temp

.L8:
# Does 32 transfers, 8 bytes each = 256 bytes total.

	mov	x1, x2
	mov	x1, x3
	mov	x1, x4
	mov	x1, x5
	mov	x1, x6
	mov	x1, x7
	mov	x1, x8
	mov	x1, x9

	mov	x2, x1
	mov	x2, x3
	mov	x2, x4
	mov	x2, x5
	mov	x2, x6
	mov	x2, x7
	mov	x2, x8
	mov	x2, x9

	mov	x1, x2
	mov	x1, x3
	mov	x1, x4
	mov	x1, x5
	mov	x1, x6
	mov	x1, x7
	mov	x1, x8
	mov	x1, x9

	mov	x1, x2
	mov	x1, x3
	mov	x1, x4
	mov	x1, x5
	mov	x1, x6
	mov	x1, x7
	mov	x1, x8
	mov	x1, x9


	sub	x0, x0, #1
	cbnz	x0, .L8

# return.
	ldp x29, x30, [sp], #16
	ret

#-----------------------------------------------------------------------------
# Name: 	StackReader
# Purpose:	Performs stack-to-register transfers.
# Params:
#	x0 = count
#-----------------------------------------------------------------------------
StackReader:
	stp	x29, x30, [sp, #-16]!

# x1 = temp

	sub	sp, sp, #64
.L9:
# Does 32 transfers, 8 bytes each = 256 bytes total.

	ldr	x1, [sp]
	ldr	x1, [sp, #8]
	ldr	x1, [sp, #16]
	ldr	x1, [sp, #24]
	ldr	x1, [sp, #32]
	ldr	x1, [sp, #40]
	ldr	x1, [sp, #48]
	ldr	x1, [sp, #56]

	ldr	x1, [sp]
	ldr	x1, [sp, #8]
	ldr	x1, [sp, #16]
	ldr	x1, [sp, #24]
	ldr	x1, [sp, #32]
	ldr	x1, [sp, #40]
	ldr	x1, [sp, #48]
	ldr	x1, [sp, #56]

	ldr	x1, [sp]
	ldr	x1, [sp, #8]
	ldr	x1, [sp, #16]
	ldr	x1, [sp, #24]
	ldr	x1, [sp, #32]
	ldr	x1, [sp, #40]
	ldr	x1, [sp, #48]
	ldr	x1, [sp, #56]

	ldr	x1, [sp]
	ldr	x1, [sp, #8]
	ldr	x1, [sp, #16]
	ldr	x1, [sp, #24]
	ldr	x1, [sp, #32]
	ldr	x1, [sp, #40]
	ldr	x1, [sp, #48]
	ldr	x1, [sp, #56]

	sub	x0, x0, #1
	cbnz	x0, .L9

	add	sp, sp, #64

# return.
	ldp x29, x30, [sp], #16
	ret

#-----------------------------------------------------------------------------
# Name: 	StackWriter
# Purpose:	Performs register-to-stack transfers.
# Params:
#	x0 = count
#-----------------------------------------------------------------------------
StackWriter:
	stp	x29, x30, [sp, #-16]!

# x1 = temp

	sub	sp, sp, #64
.L10:
# Does 32 transfers, 8 bytes each = 256 bytes total.

	str	x1, [sp]
	str	x1, [sp, #8]
	str	x1, [sp, #16]
	str	x1, [sp, #24]
	str	x1, [sp, #32]
	str	x1, [sp, #40]
	str	x1, [sp, #48]
	str	x1, [sp, #56]

	str	x1, [sp]
	str	x1, [sp, #8]
	str	x1, [sp, #16]
	str	x1, [sp, #24]
	str	x1, [sp, #32]
	str	x1, [sp, #40]
	str	x1, [sp, #48]
	str	x1, [sp, #56]

	str	x1, [sp]
	str	x1, [sp, #8]
	str	x1, [sp, #16]
	str	x1, [sp, #24]
	str	x1, [sp, #32]
	str	x1, [sp, #40]
	str	x1, [sp, #48]
	str	x1, [sp, #56]

	str	x1, [sp]
	str	x1, [sp, #8]
	str	x1, [sp, #16]
	str	x1, [sp, #24]
	str	x1, [sp, #32]
	str	x1, [sp, #40]
	str	x1, [sp, #48]
	str	x1, [sp, #56]

	sub	x0, x0, #1
	cbnz	x0, .L10

	add	sp, sp, #64

# return.
	ldp x29, x30, [sp], #16
	ret

