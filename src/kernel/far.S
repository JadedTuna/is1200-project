#include "common.S"

.text
.global farcall
farcall:
	addi	$sp, $sp, -4
	sw	$ra,  0($sp)
	# Save kernel's sp
	sw	$sp, sp_savemem

	# FIXME: this should be a variable
	# li	$sp, 0x7F007800
	li	$sp, 0x7F005000

	# Jump to user's entry point
	jalr	$a0
	nop

	# Restore kernel's sp
	lw	$sp, sp_savemem
	lw	$ra,  0($sp)
	addi	$sp, $sp, 4

	jr	$ra
	nop

.data
.global sp_savemem
sp_savemem:
	.word 0
