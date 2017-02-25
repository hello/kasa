#include <process.h>
#include <stdio.h>


int main(int argc, char **argv )
{
	char cmd[1024];
	system("if not exist \"%ProgramFiles%\\Ambarella\" (mkdir \"%ProgramFiles%\\Ambarella\")");

	if (argc > 1)
	{
		sprintf(cmd,"%s > \"%%ProgramFiles%%\\Ambarella\\setup.log\" 2>&1",argv[1]);
		system(cmd);
	}

	return 0;
}
