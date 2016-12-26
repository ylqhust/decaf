 .section .data
 .comm _Temp,76,4
 .section .rodata
 .globl	Main
 .align 4
 .type	Main, @object
 .size	Main,4
Main:
 .long	0

.LStr.S1:
 .string " 第%d行数组访问越界\n"
.LStr.S2:
 .string "%d"
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
subl	$4,%esp
jmp	.L0
pushl	$6
leal	.LStr.S0,%edx
movl	%edx,(_Temp+16)
pushl	(_Temp+16)
call	printf
addl	$8,%esp
pushl	$1
call	exit
addl	$4,%esp
.L0:
movl	$10,%edx
addl	$1,%edx
movl	%edx,(_Temp+24)
movl	(_Temp+24),%edx
imull	$4,%edx
movl	%edx,(_Temp+24)
pushl	(_Temp+24)
call	malloc
movl	%eax,(_Temp+20)
addl	$4,%esp
movl	(_Temp+24),%eax
movl	$4,%ecx
cltd
idivl	%ecx
movl	%eax,(_Temp+24)
movl	(_Temp+24),%edx
subl	$1,%edx
movl	%edx,(_Temp+24)
movl	(_Temp+24),%eax
movl	$0,%ebx
movl	(_Temp+20),%esi
movl	%eax,(%ebx,%esi)
movl	(_Temp+20),%edx
addl	$4,%edx
movl	%edx,(_Temp+8)
movl	(_Temp+8),%eax
movl	%eax,-4(%ebp)
movl	-4(%ebp),%eax
movl	%eax,(_Temp+40)
movl	$-4,%ebx
movl	(_Temp+40),%esi
movl	(%ebx,%esi),%eax
movl	%eax,(_Temp+44)
movl	$10,%eax
cmpl	(_Temp+44),%eax
setl	%al
movzbl	%al,%eax
movl	%eax,(_Temp+52)
movl	$0,%eax
cmpl	$10,%eax
setle	%al
movzbl	%al,%eax
movl	%eax,(_Temp+56)
movl	(_Temp+52),%eax
cmpl	$0,%eax
je	.L2
movl	(_Temp+56),%eax
cmpl	$0,%eax
je	.L2
movl	$1,%eax
jmp	.L3
.L2:
movl	$0,%eax
.L3:
movl	%eax,(_Temp+60)
movl	(_Temp+60),%edx
cmpl	$0,%edx
jne	.L1
pushl	$7
leal	.LStr.S1,%edx
movl	%edx,(_Temp+64)
pushl	(_Temp+64)
call	printf
addl	$8,%esp
pushl	$1
call	exit
addl	$4,%esp
.L1:
movl	$10,%edx
imull	$4,%edx
movl	%edx,(_Temp+68)
movl	(_Temp+68),%ebx
movl	(_Temp+40),%esi
movl	(%ebx,%esi),%eax
movl	%eax,(_Temp+32)
pushl	(_Temp+32)
leal	.LStr.S2,%edx
movl	%edx,(_Temp+36)
pushl	(_Temp+36)
call	printf
addl	$8,%esp
addl	$4,%esp
popl	%ebp
ret

