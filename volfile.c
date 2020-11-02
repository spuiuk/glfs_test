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

int check_volfile(glfs_t *fs)
{
        char *buf;
        size_t buflen = 4096;
        int ret;

        do {
                buf = malloc(buflen);
                if (!buf) {
                        fprintf(stderr, "Error allocating buffer\n");
                        return -1;
                }

                ret = glfs_get_volfile(fs, buf, buflen);
                if (ret == 0) {
                        fprintf(stderr, "No volfile available\n");
                } else if (ret < 0) {
                        free(buf);
                        buflen = buflen - ret;
                }
        } while(ret < 0);

        if (ret > 0) {
                printf("*%s*", buf);
        }

        return ret;
}

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
        check_volfile(fs);

done:
        glfs_fini(fs);

	return ret;
}
