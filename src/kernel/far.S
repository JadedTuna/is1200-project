#include "common.S"

.global farcall
farcall:
	# temporary, saves v0 and v1 since the handwritten code
	# in kernel.c uses them
	PUSH	$ra
	PUSH	$v0
	PUSH	$v1

	jalr	$a0
	nop

	POP		$v1	
	POP		$v0
	POP		$ra

	jr		$ra	
	nop