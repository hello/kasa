/*******************************************************************************
 * am_rtsp_authenticator.h
 *
 * History:
 *   2015-1-9 - [Shiming Dong] created file
 *
 * Copyright (C) 2008-2015, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_RTSP_AUTHENTICATOR_H_
#define AM_RTSP_AUTHENTICATOR_H_
#include <queue>
#include "am_rtsp_requests.h"

struct UserDB
{
    char *username;
    char *realm;
    char *hashcode;
    UserDB() :
        username(NULL),
        realm(NULL),
        hashcode(NULL)
    {
    }

    ~UserDB()
    {
      delete[] username;
      delete[] realm;
      delete[] hashcode;
    }

    void set_user_name(char *name)
    {
      delete[] username;
      username = amstrdup(name);
    }

    void set_realm(char *rlm)
    {
      delete[] realm;
      realm = amstrdup(rlm);
    }

    void set_hash_code(char *hash)
    {
      delete[] hashcode;
      hashcode = amstrdup(hash);
    }
};

typedef std::queue<UserDB*> UserDataBase;

struct RtspAuthHeader;
class AMRtspAuthenticator
{
  public:
    AMRtspAuthenticator();
    virtual ~AMRtspAuthenticator();

  public:
    bool init(const char *db);

  public:
    const char* get_realm();
    const char* get_nonce();
    void set_realm(const char *realm);
    void set_nonce(const char *nonce);
    bool authenticate(RtspRequest& req_header,
                      RtspAuthHeader& auth_header,
                      const char *userdb);

  private:
    char* lookup_user_hashcode(const char *user, const char *userdb);
    bool update_user_database(const char *db);

  private:
    char *m_realm;
    char *m_nonce;
    UserDataBase *m_user_db;
};

#endif /* AM_RTSP_AUTHENTICATOR_H_ */
