#include <stdio.h>
#include <stdlib.h>

int
main(int argc, char * argv[])
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s [FILE]\n", argv[0]);
        exit(1);
    }

    FILE * fp = fopen(argv[1], "r");
    char buffer[1024];
    fgets(buffer, sizeof(buffer), fp);
    fclose(fp);
}
