/*
 * list_control.cpp
 *
 * History:
 *    2014/10/13 - [Zhi He] create file
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

#include "common_base.h"

#include "app_framework_if.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN

//-----------------------------------------------------------------------
//
// CListCtl
//
//-----------------------------------------------------------------------

CListCtl *CListCtl::Create()
{
    CListCtl *result = new CListCtl();
    if (DUnlikely((result) && (EECode_OK != result->Construct()))) {
        result->Destroy();
        result = NULL;
    }
    return result;
}

CListCtl::CListCtl()
{
}

EECode CListCtl::Construct()
{
    return inherited::Construct();
}

CListCtl::~CListCtl()
{
}

EECode CListCtl::Draw(TU32 &updated)
{
    LOG_FATAL("CListCtl::Draw TO DO\n");
    return EECode_NoImplement;
}

EECode CListCtl::Action(EInputEvent event)
{
    LOG_FATAL("CListCtl::Action TO DO\n");
    return EECode_NoImplement;
}

void CListCtl::Destroy()
{
    delete this;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

