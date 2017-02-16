#ifndef __AMBA_FS__
#define __AMBA_FS__

#define FS_TYPE_FAT  0x32

int do_ls(int argc, char *argv[], int fstype);
int do_chdir(int argc, char *argv[], int fstype);
int do_fsread(const char *filename, u32 addr, int fstype, int exec);
int do_fswrite(const char *filename, u32 addr, int fstype, u32 size);
int do_info(int fstype);

#endif /* __AMBA_FS__*/
