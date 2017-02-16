/**
 * user_protocol_default_interface.cpp
 */

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

#include "user_protocol_default_if.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

TU32 gfUPDefaultCamera_BuildZoomCmd(TU8 *payload, TU32 buffer_size, TU32 offset_x, TU32 offset_y, TU32 width, TU32 height)
{
    if (DUnlikely((!payload) || (buffer_size < DUPDefaultCameraZoomCmdLength))) {
        LOG_FATAL("NULL %p or not enough size %u\n", payload, buffer_size);
        return 0;
    }

    payload[0] = EUPDefaultCameraTLV8Type_Zoom;
    payload[1] = DUPDefaultCameraZoomCmdTLVPayloadLength;
    payload += 2;

    DBEW32(offset_x, payload);
    payload += 4;
    DBEW32(offset_y, payload);
    payload += 4;
    DBEW32(width, payload);
    payload += 4;
    DBEW32(height, payload);

    LOG_NOTICE("[DEBUG]: build zoom cmd (%u %u %u %u)\n", offset_x, offset_y, width, height);

    return DUPDefaultCameraZoomCmdLength;
}

EECode gfUPDefaultCamera_ParseZoomCmd(TU8 *payload, TU32 &offset_x, TU32 &offset_y, TU32 &width, TU32 &height)
{
    if (DUnlikely(!payload)) {
        LOG_FATAL("NULL %p\n", payload);
        return EECode_BadParam;
    }

    //debug check

    DASSERT(EUPDefaultCameraTLV8Type_Zoom == payload[0]);
    DASSERT(DUPDefaultCameraZoomCmdTLVPayloadLength == payload[1]);
    payload += 2;

    DBER32(offset_x, payload);
    payload += 4;
    DBER32(offset_y, payload);
    payload += 4;
    DBER32(width, payload);
    payload += 4;
    DBER32(height, payload);

    LOG_NOTICE("[DEBUG]: parse zoom cmd (%u %u %u %u)\n", offset_x, offset_y, width, height);

    return EECode_OK;
}

TU32 gfUPDefaultCamera_BuildZoomV2Cmd(TU8 *payload, TU32 buffer_size, TU32 zoom_factor, TU32 zoom_offset_x, TU32 zoom_offset_y)
{
    if (DUnlikely((!payload) || (buffer_size < DUPDefaultCameraZoomV2CmdLength))) {
        LOG_FATAL("NULL %p or not enough size %u\n", payload, buffer_size);
        return 0;
    }

    payload[0] = EUPDefaultCameraTLV8Type_ZoomV2;
    payload[1] = DUPDefaultCameraZoomV2CmdTLVPayloadLength;
    payload += 2;

    DBEW32(zoom_factor, payload);
    payload += 4;
    DBEW32(zoom_offset_x, payload);
    payload += 4;
    DBEW32(zoom_offset_y, payload);

    LOG_NOTICE("[DEBUG]: build zoomV2 cmd (%u %u %u)\n", zoom_factor, zoom_offset_x, zoom_offset_y);

    return DUPDefaultCameraZoomV2CmdLength;
}

EECode gfUPDefaultCamera_ParseZoomV2Cmd(TU8 *payload, TU32 &zoom_factor, TU32 &zoom_offset_x, TU32 &zoom_offset_y)
{
    if (DUnlikely(!payload)) {
        LOG_FATAL("NULL %p\n", payload);
        return EECode_BadParam;
    }

    //debug check

    DASSERT(EUPDefaultCameraTLV8Type_ZoomV2 == payload[0]);
    DASSERT(DUPDefaultCameraZoomV2CmdTLVPayloadLength == payload[1]);
    payload += 2;

    DBER32(zoom_factor, payload);
    payload += 4;
    DBER32(zoom_offset_x, payload);
    payload += 4;
    DBER32(zoom_offset_y, payload);

    LOG_NOTICE("[DEBUG]: parse zoomV2 cmd (%u %u %u)\n", zoom_factor, zoom_offset_x, zoom_offset_y);

    return EECode_OK;
}

TU32 gfUPDefaultCamera_BuildBitrateCmd(TU8 *payload, TU32 buffer_size, TU32 bitrateKbps)
{
    if (DUnlikely((!payload) || (buffer_size < DUPDefaultCameraBitrateCmdLength))) {
        LOG_FATAL("NULL %p or not enough size %u\n", payload, buffer_size);
        return 0;
    }

    payload[0] = EUPDefaultCameraTLV8Type_UpdateBitrate;
    payload[1] = DUPDefaultCameraBitrateCmdTLVPayloadLength;
    payload += 2;

    DBEW32(bitrateKbps, payload);

    LOG_NOTICE("[DEBUG]: build Bitrate cmd (%u Kbps)\n", bitrateKbps);

    return DUPDefaultCameraBitrateCmdLength;
}

EECode gfUPDefaultCamera_ParseBitrateCmd(TU8 *payload, TU32 &bitrateKbps)
{
    if (DUnlikely(!payload)) {
        LOG_FATAL("NULL %p\n", payload);
        return EECode_BadParam;
    }

    //debug check

    DASSERT(EUPDefaultCameraTLV8Type_UpdateBitrate == payload[0]);
    DASSERT(DUPDefaultCameraBitrateCmdTLVPayloadLength == payload[1]);
    payload += 2;

    DBER32(bitrateKbps, payload);

    LOG_NOTICE("[DEBUG]: parse Bitrate cmd (%u Kbps)\n", bitrateKbps);

    return EECode_OK;
}

TU32 gfUPDefaultCamera_BuildFramerateCmd(TU8 *payload, TU32 buffer_size, TU32 framerate)
{
    if (DUnlikely((!payload) || (buffer_size < DUPDefaultCameraFramerateCmdLength))) {
        LOG_FATAL("NULL %p or not enough size %u\n", payload, buffer_size);
        return 0;
    }

    payload[0] = EUPDefaultCameraTLV8Type_UpdateFramerate;
    payload[1] = DUPDefaultCameraFramerateCmdTLVPayloadLength;
    payload += 2;

    DBEW32(framerate, payload);

    LOG_NOTICE("[DEBUG]: build Framerate cmd (%u fps)\n", framerate);

    return DUPDefaultCameraFramerateCmdLength;
}

EECode gfUPDefaultCamera_ParseFramerateCmd(TU8 *payload, TU32 &framerate)
{
    if (DUnlikely(!payload)) {
        LOG_FATAL("NULL %p\n", payload);
        return EECode_BadParam;
    }

    //debug check

    DASSERT(EUPDefaultCameraTLV8Type_UpdateFramerate == payload[0]);
    DASSERT(DUPDefaultCameraFramerateCmdTLVPayloadLength == payload[1]);
    payload += 2;

    DBER32(framerate, payload);

    LOG_NOTICE("[DEBUG]: parse Framerate cmd (%u fps)\n", framerate);

    return EECode_OK;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

