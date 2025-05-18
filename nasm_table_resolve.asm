cpu x64
bits 64
default rel


%idefine my_skipper %if 0
%idefine end_skipper %endif

section .text
my_func_01:
  or  rdx, rdx
  xor rax, rax
  ret

section .rodata
table_01:
  dq 0x01

section .text
my_func_02:
  not rax
  ret

section .rodata
table_02:
  dq 0x01

