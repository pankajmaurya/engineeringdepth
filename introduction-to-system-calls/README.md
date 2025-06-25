➜  introduction-to-system-calls git:(main) ✗ gcc -g hello_syscall.c -o hello_syscall

➜  introduction-to-system-calls git:(main) ✗ objdump -d hello_syscall | grep -A 20 write
10000047c: 90000001    	adrp	x1, 0x100000000 <_write+0x100000000>
100000480: 9112a021    	add	x1, x1, #0x4a8
100000484: d28001c2    	mov	x2, #0xe                ; =14
100000488: 94000005    	bl	0x10000049c <_write+0x10000049c>
10000048c: b9400be0    	ldr	w0, [sp, #0x8]
100000490: a9417bfd    	ldp	x29, x30, [sp, #0x10]
100000494: 910083ff    	add	sp, sp, #0x20
100000498: d65f03c0    	ret

Disassembly of section __TEXT,__stubs:

000000010000049c <__stubs>:
10000049c: 90000030    	adrp	x16, 0x100004000 <_write+0x100004000>
1000004a0: f9400210    	ldr	x16, [x16]
1000004a4: d61f0200    	br	x16
➜  introduction-to-system-calls git:(main) ✗ ./hello_syscall
Hello, world!
➜  introduction-to-system-calls git:(main) ✗ lldb ./hello_syscall
(lldb) target create "./hello_syscall"
Current executable set to '/Users/pankaj/TechCareer/engineeringdepth/introduction-to-system-calls/hello_syscall' (arm64).
(lldb) disassemble --name write
dyld`write:
dyld[0x180143498] <+0>:  mov    x16, #0x4 ; =4 
dyld[0x18014349c] <+4>:  svc    #0x80
dyld[0x1801434a0] <+8>:  b.lo   0x1801434c0    ; <+40>
dyld[0x1801434a4] <+12>: pacibsp 
dyld[0x1801434a8] <+16>: stp    x29, x30, [sp, #-0x10]!
dyld[0x1801434ac] <+20>: mov    x29, sp
dyld[0x1801434b0] <+24>: bl     0x1800d0fb4    ; cerror
dyld[0x1801434b4] <+28>: mov    sp, x29
dyld[0x1801434b8] <+32>: ldp    x29, x30, [sp], #0x10
dyld[0x1801434bc] <+36>: retab  
dyld[0x1801434c0] <+40>: ret    

libsystem_kernel.dylib`write:
libsystem_kernel.dylib[0x1804346ec] <+0>:  mov    x16, #0x4 ; =4 
libsystem_kernel.dylib[0x1804346f0] <+4>:  svc    #0x80
libsystem_kernel.dylib[0x1804346f4] <+8>:  b.lo   0x180434714    ; <+40>
libsystem_kernel.dylib[0x1804346f8] <+12>: pacibsp 
libsystem_kernel.dylib[0x1804346fc] <+16>: stp    x29, x30, [sp, #-0x10]!
libsystem_kernel.dylib[0x180434700] <+20>: mov    x29, sp
libsystem_kernel.dylib[0x180434704] <+24>: bl     0x180432494    ; cerror
libsystem_kernel.dylib[0x180434708] <+28>: mov    sp, x29
libsystem_kernel.dylib[0x18043470c] <+32>: ldp    x29, x30, [sp], #0x10
libsystem_kernel.dylib[0x180434710] <+36>: retab  
libsystem_kernel.dylib[0x180434714] <+40>: ret  



