#include <stdio.h>

int stack[256];
int sp = 0;

void push(int i)
{
    stack[sp] = i;
    sp++;
}

int pop()
{
    sp--;
    return stack[sp];
}

int main(void)
{
    // "int a=1; int b=2; int c=a+b;" 의 바이트코드
    unsigned char code[] = {
        0x08, // iconst_5
        0x05, // iconst_2
        0x64, // isub       → 5 - 2 = 3
        0x06, // iconst_3
        0x68, // imul       → 3 * 3 = 9
        0x04, // iconst_1
        0x6C, // idiv       → 9 / 1 = 9
        0x3C, // istore_1
    };
    int code_length = 8;
    int locals[256] = {0};

    int a = 0;
    int b = 0;

    for (int pc = 0; pc < code_length; pc++)
    {
        switch (code[pc])
        {
        case 0x04:
            push(1);
            break;
        case 0x05:
            push(2);
            break;
        case 0x06:
            push(3);
            break;
        case 0x08:
            push(5);
            break;
        case 0x3C:
            locals[1] = pop();
            break;
        case 0x3D:
            locals[2] = pop();
            break;
        case 0x3E:
            locals[3] = pop();
            break;
        case 0x1B:
            push(locals[1]);
            break;
        case 0x1C:
            push(locals[2]);
            break;
        case 0x60:
            push(pop() + pop());
            break;
        case 0x64:
            a = pop();
            b = pop();
            push(b - a);
            break;
        case 0x68:
            push(pop() * pop());
            break;
        case 0x6C:
            a = pop();
            b = pop();
            push(b / a);
            break;
        default:
            break;
        }
    }
    printf("c = %d\n", locals[1]);
}
