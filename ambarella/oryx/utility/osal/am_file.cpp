/*******************************************************************************
 * am_file.cpp
 *
 * Histroy:
 *  2012-3-7 2012 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include "am_base_include.h"
#include "am_log.h"
#include "am_define.h"
#include "am_file.h"

#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>

AMFile::AMFile(const char *file):
  m_fd(-1),
  m_is_open(false),
  m_file_name(NULL)
{
  if (file) {
    m_file_name = amstrdup(file);
  }
}

AMFile::~AMFile()
{
  delete[] m_file_name;
  if (m_fd >= 0) {
    ::close(m_fd);
  }
}

void AMFile::close()
{
  if (m_fd >= 0) {
    ::close(m_fd);
  }
  m_fd = -1;
  m_is_open = false;
}

bool AMFile::seek(long offset, AM_FILE_SEEK_POS where)
{
  bool ret = false;
  if (AM_LIKELY(m_fd >= 0)) {
    int whence = 0;
    switch(where) {
      case AMFile::AM_FILE_SEEK_SET:
        whence = SEEK_SET;
        break;
      case AMFile::AM_FILE_SEEK_CUR:
        whence = SEEK_CUR;
        break;
      case AMFile::AM_FILE_SEEK_END:
        whence = SEEK_END;
        break;
    }
    ret = (-1 != lseek(m_fd, offset, whence));
    if (AM_UNLIKELY(!ret)) {
      PERROR("lseek");
    }
  } else {
    ERROR("%s is not open!", m_file_name);
  }

  return ret;
}

bool AMFile::open(AM_FILE_MODE mode)
{
  if (AM_LIKELY(!m_is_open)) {
    int flags = -1;
    switch(mode) {
      case AM_FILE_READONLY:
        flags = O_RDONLY;
        break;
      case AM_FILE_WRITEONLY:
        flags = O_WRONLY;
        break;
      case AM_FILE_READWRITE:
        flags = O_RDWR;
        break;
      case AM_FILE_CREATE:
        flags = O_WRONLY|O_CREAT;
        break;
      case AM_FILE_CLEARWRITE:
        flags = O_WRONLY|O_TRUNC;
        break;
      default:
        ERROR("Unknown file mode!");
        break;
    }
    if (flags != -1) {
      if (m_file_name) {
        m_is_open = !((m_fd = ::open(m_file_name, flags, 0666)) < 0);
        if (AM_UNLIKELY(!m_is_open)) {
          ERROR("Failed to open %s: %s", m_file_name, strerror(errno));
        }
      } else {
        ERROR("File name not set!");
      }
    }
  }

  return m_is_open;
}

uint64_t AMFile::size()
{
  struct stat fileStat;
  uint64_t size = 0;
  if (AM_UNLIKELY(stat(m_file_name, &fileStat) < 0)) {
    ERROR("%s stat error: %s", m_file_name, strerror(errno));
  } else {
    size = fileStat.st_size;
  }

  return size;
}

ssize_t AMFile::write(void *data, uint32_t len)
{
  ssize_t ret = -1;
  if ((m_fd >= 0) && data) {
    ret = ::write(m_fd, data, len);
  } else if (m_fd < 0) {
    ERROR("File not open!");
  } else {
    ERROR("NULL pointer specified!");
  }

  return ret;
}

ssize_t AMFile::write_reliable(const void *data, size_t len)
{
  ssize_t ret = -1;
  size_t tmp_len = len;
  do {
    if (m_fd < 0) {
      ERROR("File not open!");
      break;
    }
    if (data == NULL || len == 0) {
      ERROR("Invalid paraments!");
      break;
    }
    do {
      char *tmp_data = (char *)data;
      ret = ::write(m_fd, tmp_data, tmp_len);
      if (ret == -1) {
        PERROR("write failed");
        break;
      } else if (ret < (ssize_t)tmp_len) {
        tmp_data += ret;
        tmp_len -= ret;
      } else if (ret == (ssize_t)tmp_len) {
        break;
      }
    } while (tmp_len > 0);
  } while (0);

  return ret;
}

ssize_t AMFile::read(char *buf, uint32_t len)
{
  ssize_t ret = -1;
  if ((m_fd >= 0) && buf) {
    ret = ::read(m_fd, buf, len);
    if ((ret < 0) && (errno == EINTR)) {
      ret = ::read(m_fd, buf, len);
    } else if (ret < 0) {
      ERROR("%s read error: %s", m_file_name, strerror(errno));
    }
  } else if (m_fd < 0) {
    ERROR("File not open!");
  } else {
    ERROR("NULL pointer specified!");
  }

  return ret;
}

bool AMFile::exists()
{
  bool ret = false;

  if (m_file_name) {
    ret = (0 == access(m_file_name, F_OK));
  }
  return ret;
}

bool AMFile::exists(const char *file)
{
  bool ret = false;
  if (file) {
    ret = (0 == access(file, F_OK));
  }
  return ret;
}

bool AMFile::create_path(const char *path)
{
  if (path) {
    if (access(path, F_OK) == 0) {
      return true;
    } else {
      char parent[256] = {0};
      int ret = sprintf(parent, "%s", path);
      parent[ret] = '\0';
      char *end = strrchr(parent, (int)'/');
      if (end && (end != parent)) {
        *end = '\0';
        if (create_path(parent)) {
          if (mkdir(path, S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH) == 0) {
            return true;
          } else if (errno == EEXIST) {
            return true;
          } else {
            return false;
          }
        } else {
          return false;
        }
      } else {
        if (mkdir(path, S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH) == 0) {
          return true;
        } else if (errno == EEXIST) {
          return true;
        } else {
          return false;
        }
      }
    }
  } else {
    ERROR("Cannot create NULL path!");
  }

  return false;
}

std::string AMFile::dirname(const char *file)
{
  std::string parent;

  if (AM_LIKELY(file)) {
    std::string temp = file;
    char *end = (char*)strrchr(temp.c_str(), (int)'/');
    if (AM_LIKELY(!end)) {
      parent = ".";
    } else if (AM_LIKELY(end == temp.c_str())) {
      parent = "/";
    } else if (AM_LIKELY(strlen(end) == 1)) {
      *end = '\0';
      parent = dirname(temp.c_str());
    } else {
      *end = '\0';
      parent = temp.c_str();
    }
  }

  return parent;
}

std::string AMFile::dirname(const std::string file)
{
  return dirname(file.c_str());
}

static int filter(const struct dirent* dir)
{
  bool ret = false;
  if (AM_LIKELY(dir && ('.' != dir->d_name[0]))) {
    const char *ext = strstr(dir->d_name, ".");
    ret = (ext && is_str_n_equal(ext, ".so", strlen(".so")));
  }
  return ((int)ret);
}

int AMFile::list_files(const std::string& dir, std::string*& list)
{
  int number = 0;

  list = NULL;
  if (AM_LIKELY(!dir.empty())) {
    struct dirent **namelist = NULL;
    number = scandir(dir.c_str(), &namelist, filter, alphasort);
    if (AM_LIKELY(number > 0)) {
      list = new std::string[number];
      if (AM_LIKELY(list)) {
        for (int i = 0; i < number; ++ i) {
          list[i] = dir + "/" + namelist[i]->d_name;
          free(namelist[i]);
        }
      }
      free(namelist);
    } else if (number < 0) {
      PERROR("scandir");
    }
  }

  return number;
}
