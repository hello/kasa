/**********************************************************************
 * Main.cpp
 *
 * Histroy:
 *  2011年03月17日 - [Yupeng Chang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 **********************************************************************/
#include "Daemonize.h"
#include "IPCamControlServer.h"
#include <pthread.h>

#ifdef DEBUG
IPCamControlServer * ipcamControlServer;
void signal_handler (int sig)
{
  ipcamControlServer->stop_server();
}

int main (int argc, char * argv[])
{
  ipcamControlServer = new IPCamControlServer ();
  signal (SIGTERM, signal_handler);
  signal (SIGINT, signal_handler);
  signal (SIGQUIT, signal_handler);
  ipcamControlServer->start_server(ipcamControlServer);
  delete ipcamControlServer;
  return 0;
}
#else
int main (int argc, char * argv[])
{
  Daemonize            daemonize(argv[0]);
  IPCamControlServer * ipcamControlServer = 0;
  pthread_t thread;
  if (argc > 2) {
    fprintf (stderr,
             "Usage: AmbaIPCamControlServer {[--kill | -k] | [--check | -c] }\n");
    return -1;
  } else if (argc == 2) {
    if ((strcmp(argv[1], "--kill") == 0) ||
        (strcmp(argv[1], "-k") == 0)) {
      if (daemonize.is_daemon_running()) {
        daemonize.kill_daemon ();
      }
      return 0;
    } else if ((strcmp(argv[1], "--check") == 0) ||
               (strcmp(argv[1], "-c") == 0)) {
      if (daemonize.is_daemon_running()) {
#ifdef DEBUG
        fprintf (stdout, "AmbaIPCamControlServer is running on pid %d\n", daemonize.get_pid());
#endif
        return 0;
      }
#ifdef DEBUG
      fprintf (stderr, "AmbaIPCamControlServer is not running.\n");
#endif
      return 1;
    } else {
      fprintf (stderr, "Usage: AmbaIPCamControlServer {[--kill | -k] | [--check | -c] }\n");
      return -1;
    }
  } else {
    Daemonize::DaemonStatus daemon_status;
    daemon_status = daemonize.create_daemon();
    if (daemon_status == Daemonize::ERROR) {
      return -1;
    } else if ((daemon_status == Daemonize::P_ERROR) ||
               (daemon_status == Daemonize::P_RETURN)) {
#ifdef DEBUG
      fprintf (stderr, "Parent returned\n");
#endif
      return (daemon_status == Daemonize::P_ERROR ? -2: 0);
    } else if (daemon_status == Daemonize::C_ERROR) {
#ifdef DEBUG
      fprintf (stderr, "Daemon Create failed!\n");
#endif
      return -2;
    } else if (daemon_status == Daemonize::C_RETURN) {
      /* Process has become to a daemon process,
       * start the real server now
       */
      ipcamControlServer = new IPCamControlServer ();
      pthread_create (&thread,
                      0,
                      IPCamControlServer::start_server,
                      (void *)ipcamControlServer);
    }
  }
  daemonize.keep_running ();
  /* Process exited*/
  ipcamControlServer->stop_server ();
  delete ipcamControlServer;
  return 0;
}
#endif //DEBUG
