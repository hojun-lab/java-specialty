#include <stdio.h>

int main(void)
{
    FILE *file = fopen("Hello.class", "rb");
    if (file == NULL)
    {
        return 1;
    }

    unsigned char buffer[4];

    fread(buffer, 1, 4, file);

    for (int i = 0; i < 4; i++)
    {
        printf("%02X ", buffer[i]);
    }
    printf("\n");

    unsigned char version[2];
    fread(version, 1, 2, file);
    int minor = (version[0] << 8) | version[1];
    fread(version, 1, 2, file);
    int major = (version[0] << 8) | version[1];
    printf("Version: major=%d, minor=%d\n", major, minor);

    fread(version, 1, 2, file);
    int cp_count = (version[0] << 8 | version[1]);
    printf("Constatn pool count: %d\n", cp_count);

    for (int i = 1; i < cp_count; i++)
    {
        unsigned char tag;
        fread(&tag, 1, 1, file);

        unsigned char skip[4];
        unsigned char buf[2];
        int length;
        char str[256];

        switch (tag)
        {
        case 1: // CONSTANT_Utf8
            fread(buf, 1, 2, file);
            length = (buf[0] << 8 | buf[1]);
            fread(str, 1, length, file);
            str[length] = '\0';
            printf("#%d Utf8 = %s\n", i, str);
            break;
        case 3:
        case 4: // Integer, Float
            fread(skip, 1, 4, file);
            break;
        case 5:
        case 6: // Long, Double
            fread(skip, 1, 8, file);
            break;
        case 7: // CONSTANT_class
            fread(skip, 1, 2, file);
            int name_index = (skip[0] << 8) | skip[1];
            printf("#%d Class #%d\n", i, name_index);
            break;
        case 8: // CONSTANT_String
            fread(skip, 1, 2, file);
            int string_index = (skip[0] << 8) | skip[1];
            printf("#%d String #%d\n", i, string_index);
            break;
        case 9:  // CONSTANT_Fieldref
        case 10: // CONSTANT_Methodref
            fread(skip, 1, 2, file);
            int fieldref = (skip[0] << 8) | skip[1];
            fread(skip, 1, 2, file);
            int methodref = (skip[0] << 8) | skip[1];
            printf("#%d Methodref #%d.#%d\n", i, fieldref, methodref);
            break;
        case 12: // NameAndType
            fread(skip, 1, 2, file);
            int name_idx = (skip[0] << 8) | skip[1];
            fread(skip, 1, 2, file);
            int desc_idx = (skip[0] << 8) | skip[1];
            printf("#%d NameAndType #%d:#%d\n", i, name_idx, desc_idx);
            break;
        case 15: // MethodHandle
            fread(skip, 1, 3, file);
            break;
        case 16: // MethodType
            fread(skip, 1, 2, file);
            break;
        default:
            break;
        }
    }

    fclose(file);

    return 0;
}