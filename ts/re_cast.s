 .section .data
 .comm _Temp,60,4
 .section .rodata
 .globl	A
 .align 4
 .type	A, @object
 .size	A,4
A:
 .long	0

 .globl	B
 .align 4
 .type	B, @object
 .size	B,4
B:
 .long	0

 .globl	Main
 .align 4
 .type	Main, @object
 .size	Main,4
Main:
 .long	0

.LStr.S0:
 .string "第%d行类型转换异常\n"
 .section .text
 .globl _A_New
 .type _A_New,@function
_A_New:
pushl	%ebp
movl	%esp,%ebp
pushl	$4
call	malloc
movl	%eax,(_Temp+0)
addl	$4,%esp
leal	A,%edx
movl	%edx,(_Temp+4)
movl	(_Temp+4),%eax
movl	$0,%ebx
movl	(_Temp+0),%esi
movl	%eax,(%ebx,%esi)
popl	%ebp
movl	(_Temp+0),%eax
ret

 .globl _B_New
 .type _B_New,@function
_B_New:
pushl	%ebp
movl	%esp,%ebp
pushl	$4
call	malloc
movl	%eax,(_Temp+8)
addl	$4,%esp
leal	B,%edx
movl	%edx,(_Temp+12)
movl	(_Temp+12),%eax
movl	$0,%ebx
movl	(_Temp+8),%esi
movl	%eax,(%ebx,%esi)
popl	%ebp
movl	(_Temp+8),%eax
ret

 .globl _Main_New
 .type _Main_New,@function
_Main_New:
pushl	%ebp
movl	%esp,%ebp
pushl	$4
call	malloc
movl	%eax,(_Temp+16)
addl	$4,%esp
leal	Main,%edx
movl	%edx,(_Temp+20)
movl	(_Temp+20),%eax
movl	$0,%ebx
movl	(_Temp+16),%esi
movl	%eax,(%ebx,%esi)
popl	%ebp
movl	(_Temp+16),%eax
ret

 .globl main
 .type main,@function
main:
pushl	%ebp
movl	%esp,%ebp
subl	$8,%esp
call	_A_New
movl	%eax,(_Temp+24)
movl	(_Temp+24),%eax
movl	%eax,-4(%ebp)
leal	B,%edx
movl	%edx,(_Temp+36)
movl	-4(%ebp),%eax
movl	%eax,(_Temp+40)
movl	$0,%ebx
movl	(_Temp+40),%esi
movl	(%ebx,%esi),%eax
movl	%eax,(_Temp+44)
movl	(_Temp+36),%edx
cmpl	(_Temp+44),%edx
jle	.L0
pushl	$12
leal	.LStr.S0,%edx
movl	%edx,(_Temp+48)
pushl	(_Temp+48)
call	printf
addl	$8,%esp
pushl	$1
call	exit
addl	$4,%esp
.L0:
movl	-4(%ebp),%eax
movl	%eax,(_Temp+32)
movl	(_Temp+32),%eax
movl	%eax,-8(%ebp)
addl	$8,%esp
popl	%ebp
ret

