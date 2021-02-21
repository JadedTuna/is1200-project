  # labwork.S
  # Written 2015-2017 by F Lundevall
  # Skeleton file for IS1200/IS1500 lab 1.
  # The macros PUSH and POP are in the public domain.
  # Please add your own code at the end of the file.

  #
  # Please keep the two macros PUSH and POP unchanged
  #
.macro	PUSH reg
	addi	$sp,$sp,-4
	sw	\reg,0($sp)
.endm

.macro	POP reg
	lw	\reg,0($sp)
	addi	$sp,$sp,4
.endm
  #
  # Please add your own code below this line
  #
.global hexasc
hexasc:
	andi	$a0,$a0,0xF	# ignore higher bits
	addi	$t0,$a0,-0xA	# we want to know if input is less than 10
	bgez	$t0,isletter	# if so we do things differently
	addi	$v0,$a0,0x30	# if it's less than 10 we add the offset of ascii '0'
	jr	$ra
	nop
isletter:
	addi	$v0,$t0,0x41	# if it's >10 we add the offset of ascii 'A' to the value minus 10
	jr	$ra
	nop

 # delay
.global delay
delay:
	beqz 	$a0, done
	addi	$a0,$a0,-1
	li	$t0,0xA00
inner:	addi	$t0,$t0,-1
	bgtz	$t0,inner
	nop
	b	delay
	nop
done:	jr	$ra
	nop

 # turn time to ascii representation
.global time2string
time2string:
	PUSH	$ra
	PUSH	$s0
	PUSH	$s1
	move	$s0,$a0
	move	$s1,$a1

	srl	$a0,$s1,12
	jal	hexasc
	nop
	sb	$v0,0($s0)

	srl	$a0,$s1,8
	jal	hexasc
	nop
	sb	$v0,1($s0)

	li	$t0,0x3A
	sb	$t0,2($s0)

	srl	$a0,$s1,4
	jal	hexasc
	nop
	sb	$v0,3($s0)
	move	$a0,$s1
	jal	hexasc
	nop
	sb	$v0,4($s0)

	li	$t0,0x0
	sb	$t0,5($s0)

	POP	$s1
	POP	$s0
	POP	$ra
	jr	$ra
	nop

.global enable_interrupt
enable_interrupt:
	ei
	jr $ra

