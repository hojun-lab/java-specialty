#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_CLASSES 256

typedef struct
{
    char name[256];
    int method_count;
    int field_count;
    unsigned char *bytecode;
    int bytecode_length;
} ClassFile;

ClassFile *class_table[MAX_CLASSES];
int class_count = 0;

ClassFile *load_class(const char *class_name)
{
    FILE *file = fopen(class_name, "rb");

    if (file == NULL)
    {
        printf("ClassNotFoundException: %s\n", class_name);
        return NULL;
    }

    unsigned char magic[4];
    fread(magic, 1, 4, file);

    if (magic[0] != 0xCA || magic[1] != 0xFE || magic[2] != 0xBA || magic[3] != 0xBE)
    {
        printf("ClassFormatError: invalid magic number\n");
        fclose(file);
        return NULL;
    }

    ClassFile *cls = malloc(sizeof(ClassFile));
    strcpy(cls->name, class_name);

    unsigned char skip[4];

    cls->method_count = 0;
    cls->field_count = 0;
    cls->bytecode = NULL;
    cls->bytecode_length = 0;

    printf("Loaded class: %s\n", cls->name);
    fclose(file);
    return cls;
}

ClassFile *find_class(const char *name)
{
    for (int i = 0; i < class_count; i++)
    {
        if (strcmp(class_table[i]->name, name) == 0)
        {
            return class_table[i];
        }
    }
    return NULL;
}

ClassFile *load_and_register_class(const char *name)
{
    ClassFile *cf = find_class(name);
    if (cf != NULL)
    {
        printf("Already loaded: %s\n", name);
        return cf;
    }

    cf = load_class(name);
    if (cf == NULL)
    {
        return NULL;
    }

    class_table[class_count] = cf;
    class_count++;
    return cf;
}

int main(void)
{
    ClassFile *cls = load_and_register_class("Hello.class");
    // if (cls != NULL)
    // {
    //     printf("Class name: %s\n", cls->name);
    // }

    ClassFile *cls2 = load_and_register_class("Hello.class");

    // 없는 클래스도 테스트!
    ClassFile *cls3 = load_and_register_class("NotExist.class");

    printf("\nClass table: %d classes loaded\n", class_count);
    return 0;
}