/**
 * drawer_opengl.h
 *
 * History:
 *  2014/11/09 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __DRAWER_OPENGL_H__
#define __DRAWER_OPENGL_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CIGraphicDrawerOpenGL
//
//-----------------------------------------------------------------------

class CIGraphicDrawerOpenGL
{
public:
    static CIGraphicDrawerOpenGL *Create();
    void Destroy();

protected:
    CIGraphicDrawerOpenGL();

    EECode Construct();
    virtual ~CIGraphicDrawerOpenGL();

public:
    EECode DrawMesh(CIMesh *mesh, VCOM_SMeshVert *pDeformedVerts);
    EECode DrawTextureMesh(CIMesh *mesh, VCOM_SMeshVert *pDeformedVerts);
    EECode DrawRawDataRect(VCOM_SImageData *p_imgData, SRect *p_rect);
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

