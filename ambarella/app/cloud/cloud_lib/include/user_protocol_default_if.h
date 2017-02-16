/**
 * user_protocol_default.h
 */

#ifndef __USER_PROTOCOL_DEFAULT_H__
#define __USER_PROTOCOL_DEFAULT_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

#define DUPDefaultCameraZoomCmdTLVPayloadLength 16
#define DUPDefaultCameraZoomCmdLength 18
#define DUPDefaultCameraZoomV2CmdTLVPayloadLength 12
#define DUPDefaultCameraZoomV2CmdLength 14
#define DUPDefaultCameraBitrateCmdTLVPayloadLength 4
#define DUPDefaultCameraBitrateCmdLength 6
#define DUPDefaultCameraFramerateCmdTLVPayloadLength 4
#define DUPDefaultCameraFramerateCmdLength 6

//camera related
typedef enum {
    EUPDefaultCameraTLV8Type_Invalid = 0x00,

    //
    EUPDefaultCameraTLV8Type_Standby = 0x01,
    EUPDefaultCameraTLV8Type_Streaming = 0x02,
    EUPDefaultCameraTLV8Type_DisableAudio = 0x03,
    EUPDefaultCameraTLV8Type_EnableAudio = 0x04,

    //
    EUPDefaultCameraTLV8Type_Zoom = 0x20,
    EUPDefaultCameraTLV8Type_ZoomV2 = 0x21,
    EUPDefaultCameraTLV8Type_UpdateBitrate = 0x22,
    EUPDefaultCameraTLV8Type_UpdateFramerate = 0x23,

} EUPDefaultCameraTLV8Type;

TU32 gfUPDefaultCamera_BuildZoomCmd(TU8 *payload, TU32 buffer_size, TU32 offset_x, TU32 offset_y, TU32 width, TU32 height);
EECode gfUPDefaultCamera_ParseZoomCmd(TU8 *payload, TU32 &offset_x, TU32 &offset_y, TU32 &width, TU32 &height);

TU32 gfUPDefaultCamera_BuildZoomV2Cmd(TU8 *payload, TU32 buffer_size, TU32 zoom_factor, TU32 zoom_offset_x, TU32 zoom_offset_y);
EECode gfUPDefaultCamera_ParseZoomV2Cmd(TU8 *payload, TU32 &zoom_factor, TU32 &zoom_offset_x, TU32 &zoom_offset_y);

TU32 gfUPDefaultCamera_BuildBitrateCmd(TU8 *payload, TU32 buffer_size, TU32 bitrateKbps);
EECode gfUPDefaultCamera_ParseBitrateCmd(TU8 *payload, TU32 &bitrateKbps);

TU32 gfUPDefaultCamera_BuildFramerateCmd(TU8 *payload, TU32 buffer_size, TU32 framerate);
EECode gfUPDefaultCamera_ParseFramerateCmd(TU8 *payload, TU32 &framerate);

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

