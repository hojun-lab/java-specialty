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
        0x04, // iconst_1
        0x3C, // istore_1
        0x05, // iconst_2
        0x3D, // istore_2
        0x1B, // iload_1
        0x1C, // iload_2
        0x60, // iadd
        0x3E, // istore_3
    };
    int code_length = 8;
    int locals[256] = {0};

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
        default:
            break;
        }
    }
    printf("c = %d\n", locals[3]);
    // push(1);
    // push(2);
    // int a = pop();
    // int b = pop();
    // push(a + b);
    // printf("result: %d\n", pop());
    // return 0;
}
