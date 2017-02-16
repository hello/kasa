/*
 * test_editor.cpp
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
// CTextEditor
//
//-----------------------------------------------------------------------

CTextEditor *CTextEditor::Create()
{
    CTextEditor *result = new CTextEditor();
    if (DUnlikely((result) && (EECode_OK != result->Construct()))) {
        result->Destroy();
        result = NULL;
    }
    return result;
}

CTextEditor::CTextEditor()
{
}

EECode CTextEditor::Construct()
{
    return inherited::Construct();
}

CTextEditor::~CTextEditor()
{
}

EECode CTextEditor::Draw(TU32 &updated)
{
    LOG_FATAL("CTextEditor::Draw TO DO\n");
    return EECode_NoImplement;
}

EECode CTextEditor::Action(EInputEvent event)
{
    LOG_FATAL("CTextEditor::Action TO DO\n");
    return EECode_NoImplement;
}

void CTextEditor::Destroy()
{
    delete this;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

