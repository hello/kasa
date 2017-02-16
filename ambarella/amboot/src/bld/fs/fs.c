#include <bldfunc.h>
#include "fs.h"
#include "fat.h"


struct fstype_info {
	int fstype;
	char *name;
	int (*ls)(const char *dirname);
	int (*cd)(const char *dirname);
	int (*read)(const char *filename, u32 addr, int exec);
	int (*write)(const char *filename, u32 addr, u32 size);
	int (*info)(void);
};

static struct fstype_info fstypes[] = {
	{
		.fstype = FS_TYPE_FAT,
		.name = "fat",
		.ls = file_fat_ls,
		.cd = file_fat_chdir,
		.info = file_fat_info,
		.read = file_fat_read,
		.write = file_fat_write,
	}
};

static struct fstype_info *fs_get_info(int fstype)
{
	struct fstype_info *info = NULL;
	int i;

	for (i = 0, info = fstypes; i < ARRAY_SIZE(fstypes) - 1; i++, info++) {
		if (fstype == info->fstype)
			return info;
	}

	/* Return the 'unsupported' sentinel */
	return info;
}
int do_ls(int argc, char *argv[], int fstype)
{
	struct fstype_info *fsinfo;

	fsinfo = fs_get_info(fstype);
	if(fsinfo == NULL){
		putstr("fstype is not supported\r\n");
		return -1;
	}

	fsinfo->ls(argv[1]);

	return 0;
}
int do_info(int fstype)
{
	struct fstype_info *fsinfo;

	fsinfo = fs_get_info(fstype);
	if(fsinfo == NULL){
		putstr("fstype is not supported\r\n");
		return -1;
	}

	fsinfo->info();

	return 0;

}

int do_chdir(int argc, char *argv[], int fstype)
{
	struct fstype_info *fsinfo;

	fsinfo = fs_get_info(fstype);
	if(fsinfo == NULL){
		putstr("fstype is not supported\r\n");
		return -1;
	}

	fsinfo->cd(argv[1]);

	return 0;

}
int do_fsread(const char *filename, u32 addr, int fstype, int exec)
{
	struct fstype_info *fsinfo;

	fsinfo = fs_get_info(fstype);
	if(fsinfo == NULL){
		putstr("fstype is not supported\r\n");
		return -1;
	}

	fsinfo->read(filename, addr, exec);

	return 0;
}

int do_fswrite(const char *filename, u32 addr, int fstype, u32 size)
{
	struct fstype_info *fsinfo;

	fsinfo = fs_get_info(fstype);
	if(fsinfo == NULL){
		putstr("fstype is not supported\r\n");
		return -1;
	}

	fsinfo->write(filename, addr, size);

	return 0;
}
