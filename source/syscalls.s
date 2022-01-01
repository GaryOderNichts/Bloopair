.global SCKernelCopyData
SCKernelCopyData:
	// Disable data address translation
	mfmsr %r6
	li %r7, 0x10
	andc %r6, %r6, %r7
	mtmsr %r6
	
	// Copy data
	addi %r3, %r3, -1
	addi %r4, %r4, -1
	mtctr %r5
SCKernelCopyData_loop:
	lbzu %r5, 1(%r4)
	stbu %r5, 1(%r3)
	bdnz SCKernelCopyData_loop
	
	// Enable data address translation
	ori %r6, %r6, 0x10
	mtmsr %r6
blr

.global SC_KernelCopyData
SC_KernelCopyData:
	li %r0, 0x2500
	sc
blr
