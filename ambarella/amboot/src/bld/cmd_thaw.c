#include <bldfunc.h>

static int cmd_thaw(int argc, char *argv[])
{
	if(argc != 2)
		return -1;

	if(!strcmp(argv[1], "info")){
		thaw_info();
		return 0;
	}

	if(!strcmp(argv[1], "load")){
		thaw_hibernation();
		return 0;
	}
	return -1;
}

static char help_thaw[] =
"\t thaw load \r\n"
"\t thaw info \r\n";
__CMDLIST(cmd_thaw, "thaw", help_thaw);
