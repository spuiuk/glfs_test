/*
        gcc -o test_glfs test_glfs.c -l gfapi

        We test with the following scenario.
        Two separate glusterfs connections are made to the server. We use one
        stack to write to a file on the server and the second stack to read from
        the server.

        The following operations are performed.
        1) Write a pattern to the file.
        2) Read the data back from the file.
        3) Compare the write and read buffer.

        The data written to and then read from the server should be the same.
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <glusterfs/api/glfs.h>

#define NUMOPS 1000

/* Generate read and write buffer
   We cycle through the alphabet to generate the write buffer as it is easier
   to debug with wireshark. The read buffer is zeroed out before we read to it.
 */
void gen_buf(uint8_t *buf, uint8_t *rbuf, size_t bufsize)
{
	int i;
	static char t = 'a';

	if (t == 'z') {
		t = 'a';
	} else {
		t++;
	}

	for (i = 0; i < bufsize; i++) {
		*(buf + i) = t;
		*(rbuf + i) = 0;
	}
}

int main (int argc, char** argv)
{
        uint8_t buf[131072];
        uint8_t rbuf[131072];
        char *hostname = NULL;
        char *volname = NULL;
        char *logfile = NULL;
        char *filename = "test321";
        glfs_t *fs1 = NULL, *fs2 = NULL;
        glfs_fd_t *fdw, *fdr;
        int ret=0;
        int i;
        struct stat sbuf;

        if (argc != 4) {
                fprintf(stderr, "Invalid argument.\n");
                exit(1);
        }

        hostname = argv[1];
        volname = argv[2];
        logfile = argv[3];

        fs1 = glfs_new(volname);
        if (!fs1) {
                fprintf(stderr, "glfs_new_failed fs1\n");
                exit(1);
        }
        fs2 = glfs_new(volname);
        if (!fs2) {
                fprintf(stderr, "glfs_new_failed fs2\n");
                exit(1);
        }


        ret = glfs_set_volfile_server(fs1, "tcp", hostname, 24007);
        if (ret < 0) {
                fprintf(stderr, "glfs_set_volfile_server failed fs1\n");
                goto done;
        }

        ret = glfs_set_volfile_server(fs2, "tcp", hostname, 24007);
        if (ret < 0) {
                fprintf(stderr, "glfs_set_volfile_server failed fs2\n");
                goto done;
        }


        ret = glfs_set_logging(fs1, logfile, 7);
        if (ret < 0) {
                fprintf(stderr, "glfs_set_logging failed\n");
                goto done;
        }

        ret = glfs_set_xlator_option(fs1, "*-write-behind",
				     "strict-O_DIRECT",
				     "on");
	if (ret < 0) {
		fprintf(stderr, "Failed to set xlator option: flush-behind\n");
		goto done;
	}

        ret = glfs_init(fs1);
        if (ret < 0) {
                fprintf(stderr, "glfs_init fs1 failed\n");
                return ret;
        }
        ret = glfs_init(fs2);
        if (ret < 0) {
                fprintf(stderr, "glfs_init fs2 failed\n");
                return ret;
        }

        /* We cycle through writes and reads NUMOPS times in total */

        for (i = 0; i < NUMOPS; i++) {
                size_t buf_size = ((unsigned int)random()%(sizeof(buf)-1))+ 1;
                size_t read_size;

                gen_buf(buf, rbuf, buf_size);
                fprintf(stderr, "%s:%d write size %d - pattern %c\n",
                                               __FILE__, __LINE__, buf_size, buf[0]);

                fdw = glfs_creat (fs1, filename, O_WRONLY, 0644);
                if (fdw < 0) {
                        fprintf(stderr, "Could not open file %s for writing\n", filename);
                        ret = -1;
                        goto done;
                }

                fdr = glfs_open(fs2, filename, O_RDWR|O_SYNC);
                if (fdr < 0) {
                        fprintf(stderr, "Could not open file %s for reading\n", filename);
                        ret = -2;
                        goto done;
                }
                /*
                        We had noticed a delayed open only when the actual read
                        was done. This glfs_pread call ensures that the file has
                        been opened and ready for use.
                 */
                glfs_pread(fdr, rbuf, 0, 0, 0, NULL);

                /* Step one - write a pattern to the file */
                ret = glfs_pwrite(fdw, buf, buf_size, 0, 0, NULL, NULL);
                if (ret < 0) {
                        fprintf(stderr, "Error returned when writing %d\n", ret);
                        continue;
                }
                if (ret != buf_size) {
                        fprintf(stderr, "Not enough bytes written expected - %d written - %d\n",
                                                                buf_size, ret);
                        continue;
                }

                /* Step 2 - Read back same number of bytes from the file to the read buffer */
                read_size = glfs_pread(fdr, rbuf, buf_size, 0, 0, NULL);

                /* Ignore the scenario where fewer bytes are read compared to the earlier write */
                if (read_size != buf_size) {
                        fprintf(stderr, "fewer bytes than expected read(%d) - pattern %c\n",
                                                              read_size, rbuf[0]);
                        continue;
                }

                /* Compare the write buffer with the read buffer */
                if (memcmp(rbuf, buf, buf_size)) {
                        fprintf(stderr, "read and write buffers do not match - write pattern %c - read pattern %c\n", buf[0], rbuf[0]);
                        ret = -1;
                        goto done;
                }

                /* Close and unlink the file. */
                glfs_close(fdw);
                glfs_close(fdr);
                glfs_unlink(fs1, filename);
                fprintf(stderr, "%s:%d read_size %d - pattern %c\n", __FILE__, __LINE__, read_size, rbuf[0]);
                /* Redo the test in the next iteration*/
        }
done:
        glfs_fini(fs1);
        glfs_fini(fs2);

	return ret;
}
