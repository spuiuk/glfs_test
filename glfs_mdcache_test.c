/*
 * gcc -o glfs_mdcache_test glfs_mdcache_test.c -l gfapi
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <glusterfs/api/glfs.h>

int main(int argc, char **argv)
{
	int ret;
	size_t read_size;
	char buf[40] = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
	char rbuf[40];
	size_t buf_size = 32;
	struct stat st;

	glfs_t *fs1 = NULL;
	glfs_fd_t *fd;

	char *hostname = NULL;
	char *volname = NULL;
	char *filename = "test321";

	if (argc != 3) {
		fprintf(stderr, "Invalid argument.\nUsage: %s <hostname> <volume>\n", argv[0]);
		exit(1);
	}
	hostname = argv[1];
	volname = argv[2];

	fs1 = glfs_new(volname);
	if (!fs1) {
		fprintf(stderr, "glfs_new_failed fs1\n");
		exit(1);
	}

	ret = glfs_set_volfile_server(fs1, "tcp", hostname, 24007);
	if (ret < 0) {
		fprintf(stderr, "glfs_set_volfile_server failed fs1\n");
		return ret;
	}

	ret = glfs_init(fs1);
	if (ret < 0) {
		fprintf(stderr, "glfs_init fs1 failed\n");
		return ret;
	}

	/* Delete any existing file */
	glfs_unlink(fs1, filename);

	/* Test starts here */

	/* Step 1 - create file */
	fd = glfs_creat (fs1, filename, O_RDWR, 0644);
	if (fd < 0) {
		fprintf(stderr, "Could not open file %s\n", filename);
		ret = -1;
		goto done_fini;
	}

	/* Step 2 - setxattr a pattern to the file  expected 0 */
	ret = glfs_setxattr(fs1, filename, "user.DOSATTRIB", buf, buf_size, 0);
	fprintf(stderr, "user.DOSATTRIB setxattr ret %d(0)\n", ret);

	/* Step 3 - fsetxattr security.NTACL - expected 0 */
	glfs_fsetxattr(fd, "security.NTACL", buf, buf_size, 0);
	fprintf(stderr, "security.NTACL fsetxattr ret %d(0)\n", ret);
	/* Step 4 - fgetxattr security.NTACL - expected > 0 */
	read_size = glfs_fgetxattr(fd, "security.NTACL", &rbuf, buf_size);
	fprintf(stderr, "security.NTACL read_size %d(>0)\n", (int)read_size);

	/* Step 5 - fgetxattr user.DOSATTRIB - expected >0*/
	read_size = glfs_fgetxattr(fd, "user.DOSATTRIB", &rbuf, buf_size);
	fprintf(stderr, "user.DOSATTRIB read_size %d(>0)\n", (int)read_size);
	if ((int)read_size < 0) {
		ret = read_size;
		fprintf(stderr, "Error returned when attempting fgetxattr %d\nError str: %s\n", ret, strerror(errno));
	}

	glfs_close(fd);
	glfs_unlink(fs1, filename);
done_fini:
	glfs_fini(fs1);

	return ret;
}
