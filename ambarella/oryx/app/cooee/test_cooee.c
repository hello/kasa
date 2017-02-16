#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <ctype.h>

#include "cooee_inter.h"

#ifndef MAX_TIME_TO_GET_WIFI_CONF
#define MAX_TIME_TO_GET_WIFI_CONF (10)
#endif

#ifndef DEFAULT_COOEE_CONF
#define DEFAULT_COOEE_CONF "/tmp/cooee.conf"
#endif

#define NO_ARG  0
#define HAS_ARG 1

#ifndef NAME_MAX_LEN
#define NAME_MAX_LEN (128)
#endif

static int help_flag = 0;
static int config_name_flag = 0;
static char config_name[NAME_MAX_LEN];

static const char *short_options = "f:h";
static struct option long_options[] = {
  {"help",  NO_ARG, 0, 'h'},
  {"file", HAS_ARG, 0, 'f'},
  {     0,       0, 0,  0},
};

struct hint_s {
  const char *arg;
  const char *str;
};

static const struct hint_s hint[] = {
  {"help", "\tDisplay help information"},
  {"file", "\tSpecify where ap config file to be stored"},
  {"", "\t\tPrint more information"},
};

static void usage (void)
{
  int i = 0;

  printf ("\ntest_cooee usage: \n\n");
  for (; i < sizeof (long_options) / sizeof (long_options[0]) - 1; i++) {
    if (isalpha (long_options[i].val)) {
      printf ("-%c ", long_options[i].val);
    } else {
      printf ("   ");
    }

    printf ("--%s", long_options[i].name);
    if (hint[i].arg[0] != 0) {
      printf (" [%s]", hint[i].arg);
    }

    printf ("\t%s\n", hint[i].str);
  }
}

int init_param (int argc, char **argv)
{
  int ch, ret = 0;
  int option_index = 0;

  while ((ch = getopt_long (argc, argv, short_options,
          long_options, &option_index)) != -1) {
    switch (ch) {
    case 'h':
      help_flag = 1;
      usage ();
      break;
    case 'f':
      config_name_flag = 1;
      strcpy (config_name, optarg);
      break;
    default:
      printf ("No such command: %c", ch);
      ret = -1;
      break;
    }
  }

  return ret;
}

int main(int argc, char **argv)
{
  /*
   * This test program just try to fetch wifi
   * configuration: ssid and password.
   */
  int ret = -1;
  int times = 0;

  if (argc < 2) {
    usage ();
    return 0;
  }

  if (init_param (argc, argv) < 0) {
    printf ("Failed to initialize command line.");
    return -1;
  }

  if (help_flag) {
    return 0;
  }

  if (!config_name_flag) {
    strcpy (config_name, DEFAULT_COOEE_CONF);
  }

  do {
    if ((ret = cooee_interface (config_name)) == 0) {
      printf ("cooee.conf has been generated.");
      break;
    } else {
      printf ("Timeout: %dth\n", times + 1);
    }

  } while (++times != MAX_TIME_TO_GET_WIFI_CONF);

  return 0;
}
