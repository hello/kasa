/*******************************************************************************
 * am_record_if.h
 *
 * History:
 *   2014-12-9 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/

/*! @mainpage Oryx Stream Record
 *  @section Introduction
 *  Oryx Stream Record module is used to record Video and Audio data into file
 *  container or into RTP payload for network transmission.
 *  Oryx record module supports MP4 and TS file container, and supports RTP
 *  packet.
 */

/*! @file am_record_if.h
 *  @brief AMIRecord interface
 *  This file contains AMRecord engine interface
 */

#ifndef AM_RECORD_IF_H_
#define AM_RECORD_IF_H_

#include "am_record_msg.h"
#include "am_pointer.h"
#include "version.h"

class AMIRecord;
/*! @typedef AMIRecordPtr
 *  @brief Smart pointer type used to handle AMIRecord pointer.
 */
typedef AMPointer<AMIRecord> AMIRecordPtr;

/*! @class AMIRecord
 *  @brief AMIRecord is the interface class that used to record Video and Audio
 *         into file container or into RTP packets.
 */
class AMIRecord
{
    friend AMIRecordPtr;

  public:
    /*! Constructor function.
     * Constructor static function, which creates an object of the underlying
     * object of AMIRecord class and return an smart pointer class type.
     * This function doesn't take any parameters.
     * @return AMIRecordPtr
     * @sa init()
     */
    static AMIRecordPtr create();

    /*! Initialze the object return by create()
     * @return true if success, otherwise return false
     * @sa create()
     */
    virtual bool init()                                                  = 0;

    /*! Start recording
     * @return true if success, otherwise return false
     * @sa stop();
     */
    virtual bool start()                                                 = 0;

    /*! Stop recording
     * @return true if success, otherwise return false
     * @sa start()
     */
    virtual bool stop()                                                  = 0;

    /*! Start file recording
     * @return true if success, otherwise return false
     * @sa stop_file_recording();
     */
    virtual bool start_file_recording()                                  = 0;

    /*! Stop file recording
     * @return true if success, otherwise return false
     * @sa start_file_recording()
     */
    virtual bool stop_file_recording()                                   = 0;

    /*! Test function to test if record engine is working
     * @return true if record engine is recording, otherwise return false
     * @sa is_ready_for_event()
     */
    virtual bool is_recording()                                          = 0;

    /*! Trigger an event operation inside record engine to make the engine start
     *  recording file clip to record the duration of this event.
     * @return true if success, otherwise return false
     */
    virtual bool send_event()                                            = 0;

    /*! Must be called before calling send_event(), this function tests if
     *  the record engine is currently ready for recording event file.
     * @return true if record engine is ready for recording event file,
     *         otherwise return false.
     * @sa send_event()
     * @sa is_recording()
     */
    virtual bool is_ready_for_event()                                    = 0;

    /*! Set a callback function to receive record engine messages
     * @param callback callback function type
     * @param data user data
     */
    virtual void set_msg_callback(AMRecordCallback callback, void *data) = 0;

  protected:
    /*! Decrease the reference of this object, when the reference count reaches
     *  0, destroy the object.
     * Must be implemented by the inherited class
     */
    virtual void release()                                               = 0;

    /*! Increase the reference count of this object
     */
    virtual void inc_ref()                                               = 0;

  protected:
    /*! Destructor
     */
    virtual ~AMIRecord(){}
};

/*! @example test_oryx_record.cpp
 *  Test program of AMIRecord.
 */
#endif /* AM_RECORD_IF_H_ */
