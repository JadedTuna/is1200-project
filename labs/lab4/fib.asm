
.text
	# input
	addi	$a0, $0, 8
fibonacci:
	addi	$at, $0, 1	# at the start v0 = 1
rec:
	addi	$v0, $at, 0	# put the result of v0 * a0 back in v0
	beq	$a0, $0, done	# base case
	addi	$at, $0, 0	# reset at
	addi	$a1, $a0, 0	# a1 is our counter for the loop multiplication
	addi	$a0, $a0, -1	# decrement a0
mul:		# calculate v0*a0 accumulating in at
	add	$at, $at, $v0
	addi	$a1, $a1, -1
	beq	$0, $a1, rec	# done with multiplication, back to the recursion
	beq	$0, $0, mul

done:	beq	$0, $0, done
