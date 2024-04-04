#include "user_lib.h"
#include "util/types.h"
#include "util/string.h"

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        setstatus(-1);
        printu("Too Few Arguments!!!\n");
    }
    else
    {
        int file2 = -1;
        int file1 = open(argv[0], O_RDONLY);
        if (file1 < 0)
        {
            setstatus(-1);
            printu("%s open failed!!!\n", argv[0]);
            goto over;
        }
        file2 = open(argv[1], O_RDWR| O_CREAT);
        // printu("%d %d\n",file1,file2);
        int flag = (file1 < 0) || (file2 < 0);
        if (file2 < 0)
        {
            setstatus(-1);
            printu("%s create failed!!!\n", argv[1]);
            goto over;
        }
        if (flag)
            goto over;
        struct istat stat;
        stat_u(file1, &stat);
        LibInit();
        char *buf = better_malloc(stat.st_size);
        memset(buf, 0, sizeof(buf));
        read_u(file1, buf, stat.st_size);
        printu("%d\n",write_u(file2, buf, stat.st_size));
        better_free(buf);
    over:
        if (file2 >= 0)
            close(file2);
        if (file1 >= 0)
            close(file1);
    }
    exit(0);
    return 0;
}