/*******************************************************************************
 * am_file.h
 *
 * Histroy:
 *  2012-3-7 2012 - [ypchang] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
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

#ifndef AMFILE_H_
#define AMFILE_H_

/*! @file am_file.h
 *  @brief This file defines AMFile class.
 */

/*! @class AMFile
 *  @brief A helper class used to handle files
 *  @addtogroup HelperClass
 *  @{
 */
class AMFile
{
  public:
    /*! @enum AM_FILE_MODE
     *
     */
    enum AM_FILE_MODE {
      AM_FILE_CREATE,   //!< Open file to a new one
      AM_FILE_WRITEONLY,//!< Open file with write only mode
      AM_FILE_READONLY, //!< Open file with read only mode
      AM_FILE_READWRITE,//!< Open file with read and write mode
      AM_FILE_CLEARWRITE//!< Open file in truncate mode
    };

    /*! @enum AM_FILE_SEEK_POS
     *
     */
    enum AM_FILE_SEEK_POS {
      AM_FILE_SEEK_SET,//!< Seek file from the value set by offset
      AM_FILE_SEEK_CUR,//!< Seek file from current location add by offset
      AM_FILE_SEEK_END //!< Seek file from the end minus offset
    };
  public:
    /*!
     * Constructor, takes a C style string as parameter
     * @param file C style string containing a file URI.
     */
    AMFile(const char *file);

    /*!
     * Constructor
     */
    AMFile() :
      m_file_name(NULL),
      m_fd(-1),
      m_is_open(false) {}

    /*!
     * Destructor
     */
    virtual ~AMFile();

  public:
    /*! Return the underlying file name
     * @return const C style string
     */
    const char* name()
    {
      return (const char*)m_file_name;
    }

    /*! Test to see if the file is opened
     * @return true if file is opened, otherwise false
     */
    bool is_open()
    {
      return m_is_open;
    }

    /*! Set a new file to this class
     * @param file const C style string
     */
    void set_file_name(const char *file)
    {
      delete[] m_file_name;
      m_file_name = (file ? amstrdup(file) : NULL);
    }

    /*! Seek currently opened file to "offset"
     *  from the position indicated by "where"
     * @param offset long type indicate the seek offset
     * @param where AM_FILE_SEEK_POS indicates the seek start position
     * @return true if success, otherwise return false
     */
    bool seek(long offset, AM_FILE_SEEK_POS where);

    /*! Open the file managed by this class
     * @param mode File open mode
     * @return true if file successfully opened, otherwise return false
     */
    bool open(AM_FILE_MODE mode);

    /*! Return the current offset of this file
     *  @return current file offset, if failed to get the offet, return 0
     */
    off_t offset();

    /*! Return the file size
     * @return file size, if failed to get size of the file, return 0
     */
    uint64_t size();

    /*! Close the file managed by this class
     *  @param need_sync true: call fsync before close, false: no sync
     */
    void close(bool need_sync = false);

    /*! Return the raw file descriptor of the file managed by this class
     * @return file descriptor
     */
    int handle() {return m_fd;}

    /*! Write data to file.
     * @param data pointer pointing to the data to be written.
     * @param len data length
     * @return value greater than 0 indicates the actually wrote data size
     *         in bytes, -1 if fail to write.
     */
    ssize_t write(void *data, uint32_t len);

    /*! Relatively reliable Write data to file.
     * @param data pointer pointing to the data to be written.
     * @param len data length
     * @return value greater than 0 indicates the actually wrote data size
     *         in bytes, -1 if fail to write.
     */
    ssize_t write_reliable(const void *data, size_t len);

    /*! Read data from file.
     * @param buf address where the data should be written to.
     * @param len how many bytes to read
     * @return value greater than 0 indicates the actually read data size
     *         in bytes, -1 if fail to read.
     */
    ssize_t read(char *buf, uint32_t len);

    /*! Test the file existence.
     * @return true if the file exits, otherwise return false
     */
    bool exists();

    /*! Static test function to test the file existence
     * @param file the file to be tested
     * @return true if the file exists, otherwise return false
     */
    static bool exists(const char *file);

    /*! Static function to create a path on the file system.
     * @param path indicates the file path
     * @return true if path is successfully created, otherwise return false
     */
    static bool create_path(const char *path);

    /*! Find the location where the file is.
     * @param file absolute file path.
     * @return std::string store the file location
     * @sa static std::string dirname(const std::string file);
     */
    static std::string dirname(const char *file);

    /*! Overloaded function of static bool create_path(const char *path);
     * @param file std::string containing the file path.
     * @return std::string of the file location.
     * @sa static std::string dirname(const char *file);
     */
    static std::string dirname(const std::string file);

    /*! Return a file std::string list of the given directory.
     * If files are found, list will be assigned an address pointing to a
     * std::string list containing all the files with absolute path, otherwise
     * list is NULL.
     * list MUST be deleted using delete[]
     * @param dir const std::string reference containing the directory.
     * @param list a std::string* reference to store the file list.
     * @return file numbers found in the directory, 0 indicates no files.
     */
    static int list_files(const std::string& dir, std::string*& list);

  private:
    char *m_file_name;
    int   m_fd;
    bool  m_is_open;
};

/*!
 * @}
 */

#endif /* AMFILE_H_ */
