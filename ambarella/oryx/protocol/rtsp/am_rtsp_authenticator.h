/*******************************************************************************
 * am_rtsp_authenticator.h
 *
 * History:
 *   2015-1-9 - [Shiming Dong] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
