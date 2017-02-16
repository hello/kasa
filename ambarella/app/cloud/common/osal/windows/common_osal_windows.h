/**
 * common_osal_windows.h
 *
 * History:
 *  2014/09/25 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CIClockSourceGenericWindows
//
//-----------------------------------------------------------------------
class CIClockSourceGenericWindows: public CObject, public IClockSource
{
    typedef CObject inherited;

public:
    static IClockSource *Create();
    virtual EECode Construct();
    virtual void Delete();
    virtual CObject *GetObject0() const;

protected:
    CIClockSourceGenericWindows();
    ~CIClockSourceGenericWindows();

public:
    virtual TTime GetClockTime(TUint relative = 1) const;
    virtual TTime GetClockBase() const;

    virtual void SetClockFrequency(TUint num, TUint den);
    virtual void GetClockFrequency(TUint &num, TUint &den) const;

    virtual void SetClockState(EClockSourceState state);
    virtual EClockSourceState GetClockState() const;
    virtual void UpdateTime();

    void PrintStatus();

private:
    TU8 mbUseNativeUSecond;
    TU8 msState;
    TU8 mbGetBaseTime;
    TU8 mReserved2;

    TUint mTimeUintDen;
    TUint mTimeUintNum;

    TTime mSourceCurrentTimeTick;
    TTime mSourceBaseTimeTick;

    TTime mCurrentTimeTick;

private:
    IMutex *mpMutex;
};


DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

