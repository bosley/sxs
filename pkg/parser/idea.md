
I want to make this like nabla

```asm


.init main ; the symbol to start off in "main"


.sym


i8  A  0x01
i16 B  33
i32 C  0b10111000_11111111_01010011_00111101
i64 D  44_123

f32 E  3.33
f64 F  4.21

; s[0-9].. up-to 65536

s128 G This is a string up-to 128 chars long pre-alloced .
  ; Period ends it unless escaped with a \ unused chars are nil 

; Any 

.code

; Blocks of code are seperated into 
main [
  ; instructions here

]

helper [
  ; code


:label_one


  jmp label_one

]

```


# Instructions

ia    <dest reg> <lhs reg> <rhs reg>
is
id
im

fa    <dest reg> <lhs reg> <rhs reg>
fs
fd
fm

ilt   <dest label> <lhs reg>> <rhs reg>
igt
ieq
ine
ilte
igte

flt   <dest label> <lhs reg>> <rhs reg>
fgt
feq
fne
flte
fgte

and   <dest reg> <lhs reg> <rhs reg>
or
xor
not
nop
lsh
rsh


jmp
jne 