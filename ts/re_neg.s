 .section .data
 .comm _Temp,44,4
 .section .rodata
 .globl	Main
 .align 4
 .type	Main, @object
 .size	Main,4
Main:
 .long	0

.LStr.S0:
 .string "第%d行不能用负数初始化数组\n"
 .section .text
 .globl _Main_New
 .type _Main_New,@function
_Main_New:
pushl	%ebp
movl	%esp,%ebp
pushl	$4
call	malloc
movl	%eax,(_Temp+0)
addl	$4,%esp
leal	Main,%edx
movl	%edx,(_Temp+4)
movl	(_Temp+4),%eax
movl	$0,%ebx
movl	(_Temp+0),%esi
movl	%eax,(%ebx,%esi)
popl	%ebp
movl	(_Temp+0),%eax
ret

 .globl main
 .type main,@function
main:
pushl	%ebp
movl	%esp,%ebp
subl	$8,%esp
movl	$-9,%eax
movl	%eax,-8(%ebp)
movl	-8(%ebp),%eax
movl	%eax,(_Temp+20)
movl	$0,%edx
cmpl	(_Temp+20),%edx
jl	.L0
pushl	$8
leal	.LStr.S0,%edx
movl	%edx,(_Temp+24)
pushl	(_Temp+24)
call	printf
addl	$8,%esp
pushl	$1
call	exit
addl	$4,%esp
.L0:
movl	(_Temp+20),%edx
addl	$1,%edx
movl	%edx,(_Temp+32)
movl	(_Temp+32),%edx
imull	$4,%edx
movl	%edx,(_Temp+32)
pushl	(_Temp+32)
call	malloc
movl	%eax,(_Temp+28)
addl	$4,%esp
movl	(_Temp+32),%eax
movl	$4,%ecx
cltd
idivl	%ecx
movl	%eax,(_Temp+32)
movl	(_Temp+32),%edx
subl	$1,%edx
movl	%edx,(_Temp+32)
movl	(_Temp+32),%eax
movl	$0,%ebx
movl	(_Temp+28),%esi
movl	%eax,(%ebx,%esi)
movl	(_Temp+28),%edx
addl	$4,%edx
movl	%edx,(_Temp+16)
movl	(_Temp+16),%eax
movl	%eax,-4(%ebp)
addl	$8,%esp
popl	%ebp
ret

