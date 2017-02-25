/*******************************************************************************
 * common_osal_windows.h
 *
 * History:
 *  2014/09/25 - [Zhi He] create file
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

