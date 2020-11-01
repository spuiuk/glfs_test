/*
        gcc -o volfile volfile.c -l gfapi

 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <glusterfs/api/glfs.h>

int main (int argc, char** argv)
{
        char *hostname = NULL;
        char *volname = NULL;
        char *logfile = NULL;
        glfs_t *fs = NULL;
        int ret = 0;
        char *buf;
        unsigned int buflen = 2048;

        if (argc != 4) {
                fprintf(stderr, "Invalid argument.\n");
                exit(1);
        }

        hostname = argv[1];
        volname = argv[2];
        logfile = argv[3];

        fs = glfs_new(volname);
        if (!fs) {
                fprintf(stderr, "glfs_new_failed\n");
                exit(1);
        }

        ret = glfs_set_volfile_server(fs, "tcp", hostname, 24007);
        if (ret < 0) {
                fprintf(stderr, "glfs_set_volfile_server failed fs\n");
                goto done;
        }

        ret = glfs_init(fs);
        if (ret < 0) {
                fprintf(stderr, "glfs_init fs failed\n");
                return ret;
        }

        fprintf(stderr, "attempting get_volfile\n");
        buf = malloc(buflen);
        ret = glfs_get_volfile(fs, buf, buflen);
        if (ret<0) {
                buflen = buflen - ret;
                free(buf);
                buf = malloc(buflen);
                ret = glfs_get_volfile(fs, buf, buflen);
        }

        fprintf(stderr, "attempting get_volfile ret %d\n", ret);


        printf("*%s*", buf);

done:
        glfs_fini(fs);

	return ret;
}
