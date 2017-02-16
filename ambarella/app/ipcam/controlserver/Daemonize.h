/*******************************************************************************
 * Daemonize.h
 *
 * History:
 *  2011年03月22日 - [Yupeng Chang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#ifndef DAEMONIZE_H
#define DAEMONIZE_H

#include <stdio.h>
#include <stdlib.h>

#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <libdaemon/daemon.h>

class Daemonize {
public:
    enum DaemonStatus {ERROR, P_ERROR, C_ERROR, P_RETURN, C_RETURN};
    Daemonize              ( const char * daemonName);
    ~Daemonize             ( );
public:
    bool is_daemon_running ( );
    bool kill_daemon       ( );
    void keep_running      ( );
    const char * get_daemon_name () { return mDaemonName;}
    DaemonStatus create_daemon ( );
    pid_t get_pid              ( ) {return mPid;}
private:
    static const char * pid_file_proc(void);
private:
    pid_t mPid;
    char  mDaemonName[256];
    bool  mRunning;
};

#endif //DAEMONIZE_H

