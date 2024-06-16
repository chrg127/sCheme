#include "scheme.h"

static char *read_file(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (!file) {
        perror("error");
        exit(1);
    }

    fseek(file, 0l, SEEK_END);
    long size = ftell(file);
    rewind(file);

    char *buf = malloc(size + 1);
    if (!buf) {
        perror("error");
        fclose(file);
        exit(1);
    }

    size_t bread = fread(buf, sizeof(char), size, file);
    if (bread < (size_t)size) {
        fprintf(stderr, "error: couldn't read file %s\n", path);
        free(buf);
        fclose(file);
        exit(1);
    }

    buf[bread] = '\0';
    fclose(file);
    return buf;
}

int main(int argc, char *argv[])
{
    if (argc == 1) {
        repl();
    } else if (argc == 3 && strcmp(argv[1], "-s") == 0) {
        exec_string(argv[2]);
    } else if (argc == 3 && strcmp(argv[1], "-f") == 0) {
        char *contents = read_file(argv[2]);
        exec_string(contents);
        free(contents);
    } else {
        printf("usage: %s OR %s -s [string] OR %s [file]\n",
            argv[0], argv[0], argv[0]);
        return 1;
    }
    return 0;
}

