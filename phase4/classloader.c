#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MAX_CLASSES 256

typedef struct
{
    char name[256];
    char descriptor[256]; // 예: "(Ljava/lang/String;)V"
} MethodInfo;

typedef struct
{
    char name[256];
    int method_count;
    MethodInfo methods[32];
    int field_count;
    unsigned char *bytecode;
    int bytecode_length;
} ClassFile;

typedef struct ClassLoader
{
    char name[64];
    char classpath[256];
    struct ClassLoader *parent;
} ClassLoader;

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

MethodInfo *resolve_method(const char *class_name, const char *method_name, const char *descriptor)
{
    ClassFile *classFile = find_class(class_name);
    if (classFile == NULL)
    {
        return NULL;
    }

    for (int i = 0; i < classFile->method_count; i++)
    {
        if (strcmp(classFile->methods[i].name, method_name) == 0)
        {
            return &classFile->methods[i];
        }
    }

    printf("NoSuchMethodError: %s.%s\n", class_name, method_name);
    return NULL;
}

ClassFile *classloader_load(ClassLoader *loader, const char *name)
{
    ClassFile *cls = find_class(name);
    if (cls != NULL)
    {
        return cls;
    }

    if (loader->parent != NULL)
    {
        cls = classloader_load(loader->parent, name);
        if (cls != NULL)
        {
            return cls;
        }
    }

    printf("[%s] searching: %s\n", loader->name, name);
    char path[512];
    sprintf(path, "%s%s", loader->classpath, name);

    cls = load_class(path);
    if (cls != NULL)
    {
        class_table[class_count] = cls;
        class_count++;
        return cls;
    }

    return NULL;
}

int main(void)
{
    ClassLoader bootstrap = {"bootstrap", "/system/", NULL};
    ClassLoader app = {"app", "./", &bootstrap};

    // CNFE: 동적 로딩 시도 — 없는 클래스를 직접 요청
    printf("=== Test 1: CNFE ===\n");
    ClassFile *plugin = classloader_load(&app, "MyPlugin.class");
    if (plugin == NULL)
        printf("→ Caught: ClassNotFoundException\n");

    // NCDFE: 있어야 하는 클래스가 없음
    printf("\n=== Test 2: NCDFE ===\n");
    ClassFile *hello = classloader_load(&app, "Hello.class");
    // Hello가 참조하는 클래스를 resolve하려는데 없음!
    MethodInfo *m = resolve_method("Missing.class", "doSomething", "()V");
    if (m == NULL)
        printf("→ Caught: NoClassDefFoundError (Missing was expected)\n");

    return 0;
}