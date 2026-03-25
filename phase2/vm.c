#include <stdio.h>

struct Frame
{
    int stack[256];
    int sp;
    int locals[256];
    int pc;
};

struct Frame frames[64];
int frame_count = 0;

void push(int i)
{
    struct Frame *f = &frames[frame_count - 1];
    f->stack[f->sp] = i;
    f->sp++;
}

int pop()
{
    struct Frame *f = &frames[frame_count - 1];
    f->sp--;
    return f->stack[f->sp];
}

int main(void)
{
    unsigned char add_code[] = {
        0x1A, // iload_0
        0x1B, // iload_1
        0x60, // iadd
        0xAC, // ireturn
    };

    unsigned char main_code[] = {
        0x06, // iconst_3
        0x07, // iconst_4
        0xB8, // invokestatic
        0x3C, // istore_1
    };
    int code_length = 16;

    int a = 0;
    int b = 0;
    int offset = 0;
    int jump_pc = 0;

    frames[0].sp = 0;
    frames[0].pc = 0;
    frame_count = 1;

    unsigned char *current_code = main_code;
    int current_lenght = 4;

    for (int pc = 0; pc < current_lenght; pc++)
    {
        struct Frame *cur = &frames[frame_count - 1];

        switch (current_code[pc])
        {
        case 0x03:
            push(0);
            break;
        case 0x04:
            push(1);
            break;
        case 0x05:
            push(2);
            break;
        case 0x06:
            push(3);
            break;
        case 0x07:
            push(4);
            break;
        case 0x08:
            push(5);
            break;
        case 0x1A:
            push(cur->locals[0]);
            break;
        case 0x3C:
            cur->locals[1] = pop();
            break;
        case 0x3D:
            cur->locals[2] = pop();
            break;
        case 0x3E:
            cur->locals[3] = pop();
            break;
        case 0x1B:
            push(cur->locals[1]);
            break;
        case 0x1C:
            push(cur->locals[2]);
            break;
        case 0xA0:
            jump_pc = pc;
            offset = (current_code[pc + 1] << 8) | current_code[pc + 2];
            b = pop();
            a = pop();
            if (a != b)
            {
                pc = jump_pc + offset - 1;
            }
            else
            {
                pc += 2;
            }
            break;
        case 0xA7:
            jump_pc = pc;
            offset = (current_code[pc + 1] << 8) | current_code[pc + 2];
            pc = jump_pc + offset - 1;
            break;
        case 0xB8:
            cur->pc = pc;
            b = pop();
            a = pop();
            frame_count++;
            frames[frame_count - 1].sp = 0;
            frames[frame_count - 1].locals[1] = a;
            frames[frame_count - 1].locals[0] = b;
            current_code = add_code;
            current_lenght = 4;
            pc = -1;
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
        case 0xAC:
            a = pop();
            frame_count--;
            push(a);
            current_code = main_code;
            current_lenght = 4;
            pc = frames[frame_count - 1].pc;
            break;
        default:
            break;
        }
    }
    printf("result = %d\n", frames[0].locals[1]);
}
