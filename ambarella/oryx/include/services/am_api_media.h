/*******************************************************************************
 * am_api_media.h
 *
 * History:
 *   2015-2-25 - [ccjing] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
/*! @file am_api_media.h
 *  @brief This header file contains a class used to add file to
 *         playback in the media service.
 */
#ifndef AM_API_MEDIA_H_
#define AM_API_MEDIA_H_

#include <string>
#include "commands/am_api_cmd_media.h"

/*! @defgroup airapi-datastructure-media Data Structure of Media Service
 *  @ingroup airapi-datastructure
 *  @brief All Oryx Media Service related method call data structures
 *  @{
 */

/*! @class am_api_playback_audio_file_list_t
 *  @brief This class is used as the parameter of the method_call function.
 *         method_call function is contained in the AMAPIHelper class which
 *         is used to interact with oryx.
 */
class AMIApiPlaybackAudioFileList
{
  public:
    /*! Create function.
     */
    static AMIApiPlaybackAudioFileList* create();
    /*! Create function.
     */
    static AMIApiPlaybackAudioFileList* create(AMIApiPlaybackAudioFileList*
                                               audio_file);
    /*! Destructor function
     */
    virtual ~AMIApiPlaybackAudioFileList(){}
    /*! This function is used to add file to the class.
     *  @param The param is file name. The max length of it is 490 bytes
     *  @return true if success, otherwise return false.
     */
    virtual bool add_file(const std::string &file_name) = 0;
    /*! This function is used to get file which was added to this class before.
     *  @param The param is file number. For example, 1 means the first file.
     *  @return true if file number is valid, otherwise false.
     */
    virtual std::string get_file(uint32_t file_number) = 0;
    /*! This function is used to get how many files in the class.
     *  @return Return how many files contained in this class.
     */
    virtual uint32_t get_file_number() = 0;
    /*! This function is used to check the file list full or not.
     * @return if it is full, return true, otherwise return false;
     */
    virtual bool is_full() = 0;
    /*! This function is used to get the whole file list.
     * @return return the address of the file list.
     */
    virtual char* get_file_list() = 0;
    /*! This function is used to get the size of file list.
     * @return return the size of the file list.
     */
    virtual uint32_t get_file_list_size() = 0;
    /*! This function is used to clear all files in the class.
     */
    virtual void clear_file() = 0;
};

/*!
 * @}
 */
#endif /* AM_API_MEDIA_H_ */
