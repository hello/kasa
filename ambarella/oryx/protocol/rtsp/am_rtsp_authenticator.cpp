/*******************************************************************************
 * am_rtsp_authenticator.cpp
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
 *
 ******************************************************************************/
#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "openssl/md5.h"
#include "am_rtsp_client_session.h"
#include "am_rtsp_authenticator.h"
#include "am_rtsp_server_config.h"

#define USER_DATA_FILE ((const char*)"/etc/webpass.txt")

AMRtspAuthenticator::AMRtspAuthenticator() :
  m_realm(NULL),
  m_nonce(NULL),
  m_user_db(NULL)
{
}

AMRtspAuthenticator::~AMRtspAuthenticator()
{
  delete[] m_realm;
  delete[] m_nonce;
  while (m_user_db && !m_user_db->empty()) {
    delete m_user_db->front();
    m_user_db->pop();
  }
  delete m_user_db;
}

bool AMRtspAuthenticator::init(const char *db)
{
  return update_user_database(db);
}

const char* AMRtspAuthenticator::get_realm()
{
  if (AM_UNLIKELY(!m_user_db || m_user_db->empty())) {
    set_realm("ambarella-s2lm");
  } else {
    set_realm(m_user_db->front()->realm);
  }
  return m_realm;
}

const char* AMRtspAuthenticator::get_nonce()
{
  return m_nonce;
}

void AMRtspAuthenticator::set_realm(const char *realm)
{
  delete[] m_realm;
  m_realm = NULL;
  m_realm = amstrdup(realm);
}

void AMRtspAuthenticator::set_nonce(const char *nonce)
{
  delete[] m_nonce;
  m_nonce = NULL;
  m_nonce = amstrdup(nonce);
}

bool AMRtspAuthenticator::authenticate(RtspRequest& req_header,
                                       RtspAuthHeader& auth_header,
                                       const char *userdb)
{
  bool ret = false;

  if (AM_LIKELY(auth_header.is_ok())) {
    /*
     * MD5(<userHash>:<nonce>:MD5(<command>:<uri>))
     */
    char *user_hash = lookup_user_hashcode(auth_header.username, userdb);
    if (AM_LIKELY(user_hash)) {
      unsigned char data[128] = {0};
      unsigned char data_hash[48] = {0};
      char hash_string[48] = {0};

      sprintf((char*)data, "%s:%s", req_header.command, auth_header.uri);
      MD5(data, strlen((const char*)data), data_hash);
      for (uint32_t i = 0; i < 16; ++ i) {
        sprintf(&hash_string[strlen(hash_string)], "%02x", data_hash[i]);
      }
      /*NOTICE("MD5(%s) = %s", data, hashString);*/
      sprintf((char*)data, "%s:%s:%s",
          user_hash, auth_header.nonce, hash_string);
      MD5(data, strlen((const char*)data), data_hash);
      memset(hash_string, 0, sizeof(hash_string));
      for (uint32_t i = 0; i < 16; ++ i) {
        sprintf(&hash_string[strlen(hash_string)], "%02x", data_hash[i]);
      }
      /*NOTICE("MD5(%s) = %s", data, hashString);*/

      ret = is_str_equal(hash_string, auth_header.response);
      NOTICE("\nServer response: %s\nClient response: %s\nAuthentication %s!",
          hash_string, auth_header.response, (ret ? "Successfully": "Failed"));
    } else {
      NOTICE("%s is not registered in this server!", auth_header.username);
    }
  }

  return ret;
}

char* AMRtspAuthenticator::lookup_user_hashcode(const char *user,
                                                const char *userdb)
{
  char *hash = NULL;

  if (AM_LIKELY(user && update_user_database(userdb) && !m_user_db->empty())) {
    size_t size = m_user_db->size();
    for (size_t i = 0; i < size; ++ i) {
      UserDB* db = m_user_db->front();
      m_user_db->pop();
      m_user_db->push(db);
      if (AM_LIKELY(is_str_same(user, db->username))) {
        hash = db->hashcode;
        NOTICE("Found user: %s@%s", db->username, db->realm);
        break;
      }
    }
  }

  return hash;
}

bool AMRtspAuthenticator::update_user_database(const char *userdb)
{
  bool ret = true;
  FILE *db = userdb ? fopen(userdb, "r") : NULL;

  if (AM_LIKELY(!m_user_db)) {
    m_user_db = new UserDataBase();
  }
  if (AM_LIKELY(db && m_user_db)) {
    /* Purge previous data base */
    while (!m_user_db->empty()) {
      delete m_user_db->front();
      m_user_db->pop();
    }
    while (!feof(db)) {
      char buf[512] = { 0 };
      if (AM_LIKELY((1 == fscanf(db, "%[^\n]\n", buf)) && !ferror(db))) {
        char user[strlen(buf)];
        char realm[strlen(buf)];
        char hash[strlen(buf)];
        memset(user, 0, sizeof(user));
        memset(realm, 0, sizeof(realm));
        memset(hash, 0, sizeof(hash));
        buf[strlen(buf)] = ':';
        if (AM_UNLIKELY(3 != sscanf(buf, "%[^:]:%[^:]:%[^:]:",
                                    user, realm, hash))) {
          ERROR("Malformed user data:\n%s\n%s %s %s", buf, user, realm, hash);
        } else {
          UserDB* user_db = new UserDB();
          user_db->set_user_name(user);
          user_db->set_realm(realm);
          user_db->set_hash_code(hash);
          m_user_db->push(user_db);
          INFO("Get User Info: %s, %s, %s", user, realm, hash);
        }
      } else {
        if (AM_LIKELY(ferror(db))) {
          PERROR("fscanf");
        } else {
          ERROR("Broken user data!");
        }
      }
    }
  } else {
    if (AM_LIKELY(!db)) {
      PERROR("fopen");
    }
    if (AM_LIKELY(!m_user_db)) {
      ERROR("Failed to new user data base.");
    }
    ret = false;
  }

  if (AM_LIKELY(db)) {
    fclose(db);
  }

  return ret;
}
