#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;
//内存全局变量
BYTE memory[32768];
//前四个为数据寄存器，后四个为地址寄存器 
WORD ax[9];
WORD ir, ip;
short flag;
//把文件中的内容存入内存
void initialize_memory();
//程序结束时输出内存的内容
void dump_memory();
//每次指令结束输出寄存器内容
void dump_register();
//把ip对应地址处连续2个字节存入ir中，然后ip+4
void get_instruction();
//翻译指令，根据函数指针表判断ir的前8位，调用对应的函数
void translate_instruction();
//从内存中取连续两字节内容
WORD get_word(WORD addr);
//从内存中连续取四字节内容
DWORD get_dword(WORD addr);
//获取当前ir对应的立即数
short get_signed_immediate();
//获取当前ir对应的立即数(无符号)
WORD get_unsigned_immediate();

int main(void)
{
    initialize_memory();
    while (true)
    {
        get_instruction();
        translate_instruction();
        dump_register();
    }
    return 0;
}

void initialize_memory() 
{
    FILE* fp = fopen("dict.dic", "r");
    int c, bit = 0, byte = 0;
    while ((c = getc(fp)) && c != EOF) {
        if (c != '0' && c != '1')
        {
            continue;
        }
        //为了获得补码
        memory[byte] = (DWORD)memory[byte] << 1u | (BYTE)(c - '0');
        if (++bit % 8 == 0)
            ++byte;
    }
    fclose(fp);
}

void dump_memory()
{
    puts("\ncodeSegment :");
    for (int i = 0; i < 512; i += 4)
    {
        printf("%d", get_dword(i));
        if ((i + 4) % 32 == 0)
            putchar('\n');
        else
            putchar(' ');
    }
    puts("\ndataSegment :");
    for (int i = 16384; i < 16896; i += 2)
    {
        printf("%hd", get_word(i));
        if ((i + 2) % 32 == 0)
            putchar('\n');
        else
            putchar(' ');
    }
}

void get_instruction()
{
    ir = get_word(ip);
    ip += 4;
}

WORD get_word(WORD addr)
{
    return ((DWORD)memory[addr] << 8u | memory[addr + 1]);
}

DWORD get_dword(WORD addr) 
{
    return ((DWORD)memory[addr] << 24u) |
       ((DWORD)memory[addr + 1] << 16u) | 
        ((DWORD)memory[addr + 2] << 8u) |
        memory[addr + 3];
}

short get_signed_immediate()
{
    return get_word(ip - 2);
}

WORD get_unsigned_immediate()
{
    return get_word(ip - 2);
}
//停机指令，并输出
void Stop()
{
    dump_register();
    dump_memory();
    exit(0);
}

void Add()
{
    BYTE r1, r2;
    r1 = (ir & 0b11110000u) >> 4u;
    r2 = ir & 0b1111u;
    if (r2 == 0) {
        short immediate = get_signed_immediate();
        ax[r1] = (short)ax[r1] + immediate;
    }
    else
    {
        ax[r1] = (short)ax[r1] + (short)get_word(ax[r2]);
    }
}
//减法运算
void Sub()
{
    BYTE r1, r2;
    r1 = (ir & 0b11110000u) >> 4u;
    r2 = ir & 0b1111u;
    if (r2 == 0)
    {
        short immediate = get_signed_immediate();
        ax[r1] = (short)ax[r1] - immediate;
    }
    else
    {
        ax[r1] = (short)ax[r1] - (short)get_word(ax[r2]);
    }
}
//乘法运算
void Mul()
{
    BYTE r1, r2;
    r1 = (ir & 0b11110000u) >> 4u;
    r2 = ir & 0b1111u;
    if (r2 == 0)
    {
        short immediate = get_signed_immediate();
        ax[r1] = (short)ax[r1] * immediate;
    }
    else
    {
        ax[r1] = (short)ax[r1] * (short)get_word(ax[r2]);
    }
}
//除法运算
void Div()
{
    BYTE r1, r2;
    r1 = (ir & 0b11110000u) >> 4u;
    r2 = ir & 0b1111u;
    if (r2 == 0)
    {
        short immediate = get_signed_immediate();
        ax[r1] = (short)ax[r1] / immediate;
    }
    else
    {
        ax[r1] = (short)ax[r1] / (short)get_word(ax[r2]);
    }
}
//判断后进行无条件跳转或条件跳转，跳转时需要回退ip+4的结果。
void Jmp()
{
    BYTE op = ir & 0b1111u;
    short immediate = get_signed_immediate();
    if (op == 0 || (op == 1 && flag == 0) || (op == 2 && flag == 1) || (op == 3 && flag == -1))
    {
        ip += immediate - 4;
    }
}
//输入一个数到寄存器 
void Read()
{
    puts("in:");
    BYTE r = (ir & 0b11110000u) >> 4u;
    scanf("%hd", &ax[r]);
}
//输出某个寄存器的内容 
void Write()
{
    printf("out: ");
    BYTE r = (ir & 0b11110000u) >> 4u;
    printf("%hd\n", ax[r]);
}
//寄存器进行比较
void Cmp()
{
    BYTE r1, r2;
    r1 = (ir & 0b11110000u) >> 4u;
    r2 = ir & 0b1111u;
    short rhs;
    if (r2 == 0)
    {
        short immediate = get_signed_immediate();
        rhs = immediate;
    }
    else
    {
        rhs = get_word(ax[r2]);
    }
    if ((short)ax[r1] == rhs)
    {
        flag = 0;
    }
    else if ((short)ax[r1] < rhs)
    {
        flag = -1;
    }
    else
    {
        flag = 1;
    }
}
//数据的传送
void Load()
{
    BYTE r1, r2;
    r1 = (ir & 0b11110000u) >> 4u;
    r2 = ir & 0b1111u;
    if (r2 == 0)
    {
        WORD immediate = get_unsigned_immediate();
        ax[r1] = immediate;
    }
    else if (1 <= r1 && r1 <= 4)
    {
        ax[r1] = get_word(ax[r2]);
    }
    else
    {
        memory[ax[r1]] = (ax[r2] & 0xff00u) >> 8u;
        memory[ax[r1] + 1] = ax[r2] & 0xffu;
    }
}
//逻辑 与 匀速 
void And()
{
    BYTE r1, r2;
    r1 = (ir & 0b11110000u) >> 4u;
    r2 = ir & 0b1111u;
    if (r2 == 0)
    {
        WORD immediate = get_unsigned_immediate();
        ax[r1] = ax[r1] && immediate;
    }
    else
    {
        ax[r1] = ax[r1] && get_word(ax[r2]);
    }
}
//逻辑 或 运算 
void Or()
{
    BYTE r1, r2;
    r1 = (ir & 0b11110000u) >> 4u;
    r2 = ir & 0b1111u;
    if (r2 == 0)
    {
        WORD immediate = get_unsigned_immediate();
        ax[r1] = ax[r1] || immediate;
    }
    else
    {
        ax[r1] = ax[r1] || get_word(ax[r2]);
    }
}
//逻辑 非 运算 
void Not()
{
    BYTE r1, r2;
    r1 = (ir & 0b11110000u) >> 4u;
    r2 = ir & 0b1111u;
    if (r2 == 0)
    {
        ax[r1] = !ax[r1];
    }
    else
    {
        WORD value = get_word(ax[r2]);
        value = !value;
        memory[ax[r2]] = (value & (0xff00u)) >> 8u;
        memory[ax[r2] + 1] = value & (0xffu);
    }
}
//指令结束时输出寄存器内容 
void dump_register()
{
    printf("ip = %hu\nflag = %hd\nir = %hu\n", ip, flag, ir);
    for (int i = 1; i <= 8; ++i)
    {
        printf("ax%d = %hd", i, ax[i]);
        if (i == 4 || i == 8)
        {
            putchar('\n');
        }
        else
        {
            putchar(' ');
        }
    }
}
//翻译指令，判断ir的前8位 
void translate_instruction()
{
    void (*table[])() = { &Stop, &Load, &Add, &Sub, &Mul,
        &Div, &And, &Or, &Not, &Cmp, &Jmp, &Read, &Write };
    (*table[(ir & 0b1111111100000000u) >> 8u])();
}