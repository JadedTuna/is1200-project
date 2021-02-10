#include "common.S"

.macro isr_loader_default idx
	.align 4
	# vector_(new)_idx are required for the linker
	# "ax" - allocatable, executable
	# @progbits - section contains data
	# without "ax",@progbits section would not be allocated in
	# memory as it is not a standard name
	.section .vector_new_\idx,"ax",@progbits
	.global __vector_\idx
	__vector_\idx:
		movi	$k0, _isr_table
		# NOTE: in Single Vector Mode, all IRQ will be forwarded
		# to __vector_0
		lw		$k0, \idx * 4($k0)
		addi	$k1, $zero, \idx
		jr		$k0
			nop
.endm


# ISR loaders
.altmacro
.align 4
.global _isr_loaders
_isr_loaders:
.set	i, 0
.rept	64
	isr_loader_default	%i
	.set				i, i+1
.endr

# ISR addresses
.text
.align 4
.global _isr_table
_isr_table:
.set	i, 0
.rept	64
	.word		_default_isr
	.set		i, i+1
.endr

# Default ISR
.align 4
# explicitly handle hazards
.set noreorder
.global _default_isr
_default_isr:
	# disable $at
	.set noat

	# caller-save registers, and some others since this
	# is an ISR without shadow registers (most likely)
	addi	$sp, $sp, -72
	sw		$ra,  0($sp)
	sw		$at,  4($sp)

	sw		$v0,  8($sp)
	sw		$v1, 12($sp)

	sw		$a0, 16($sp)
	sw		$a1, 20($sp)
	sw		$a2, 24($sp)
	sw		$a3, 28($sp)

	sw		$t0, 32($sp)
	sw		$t1, 36($sp)
	sw		$t2, 40($sp)
	sw		$t3, 44($sp)
	sw		$t4, 48($sp)
	sw		$t5, 52($sp)
	sw		$t6, 56($sp)
	sw		$t7, 60($sp)
	sw		$t8, 64($sp)
	sw		$t9, 68($sp)

	addi	$a0, $k1, 0
	jal		default_isr
		nop

	lw		$t9, 68($sp)
	lw		$t8, 64($sp)
	lw		$t7, 60($sp)
	lw		$t6, 56($sp)
	lw		$t5, 52($sp)
	lw		$t4, 48($sp)
	lw		$t3, 44($sp)
	lw		$t2, 40($sp)
	lw		$t1, 36($sp)
	lw		$t0, 32($sp)

	lw		$a3, 28($sp)
	lw		$a2, 24($sp)
	lw		$a1, 20($sp)
	lw		$a0, 16($sp)

	lw		$v1, 12($sp)
	lw		$v0,  8($sp)

	lw		$at,  4($sp)
	lw		$ra,  0($sp)
	addi	$sp, $sp, 72

	# enable $at
	.set at
	eret
		nop


# Handle exceptions (traps, syscalls, etc)
# for explanation of "ax",@progbits see isr_stub
.section .gen_handler,"ax",@progbits
# explicitly handle hazards
.set noreorder
.ent _gen_exception
_gen_exception:
	addi	$k0, $ra, 0
	addi	$k1, $a0, 0
	mfc0	$a0, $13, 0
		ehb
	srl		$a0, $a0, 2
	jal		exception_handler
		nop
	addi	$a0, $k1, 0
	addi	$ra, $k0, 0
	eret
		nop

.end _gen_exception
