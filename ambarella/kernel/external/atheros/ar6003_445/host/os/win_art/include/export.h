#ifndef EXPORT_DLL_HEADER

#define EXPORT_DLL_HEADER

#define DEVDRV_INTERFACE_SDIO       1
#define DEVDRV_INTERFACE_ETH        2
#define DEVDRV_INTERFACE_UART       3

#define USE_ART_AGENT       0xffffffff

__declspec(dllexport) A_BOOL  __cdecl loadDriver_ENE(A_BOOL bOnOff, A_UINT32 subSystemID);

__declspec(dllexport) A_BOOL __cdecl closehandle(void);

__declspec(dllexport) A_BOOL __cdecl InitAR6000_ene(HANDLE *handle, A_UINT32 *nTargetID);

__declspec(dllexport) A_BOOL __cdecl InitAR6000_eneX(HANDLE *handle, A_UINT32 *nTargetID, A_UINT8 devdrvInterface);

__declspec(dllexport) A_BOOL __cdecl EndAR6000_ene(int devIndex);

__declspec(dllexport) HANDLE __cdecl open_device_ene(A_UINT32 device_fn, A_UINT32 devIndex, char * pipeName);

__declspec(dllexport) int __cdecl  DisableDragonSleep(HANDLE device);

__declspec(dllexport) DWORD __cdecl DRG_Write(  HANDLE COM_Write, PUCHAR buf, ULONG length );

__declspec(dllexport) DWORD __cdecl DRG_Read( HANDLE pContext, PUCHAR buf, ULONG length, PULONG pBytesRead);

__declspec(dllexport) DWORD __cdecl BT_DRG_Write(  HANDLE COM_Write, PUCHAR buf, ULONG length );

__declspec(dllexport) DWORD __cdecl BT_DRG_Read( HANDLE pContext, PUCHAR buf, ULONG length, PULONG pBytesRead);

__declspec(dllexport) int  __cdecl BMIWriteSOCRegister_win(HANDLE handle, A_UINT32 address, A_UINT32 param);

__declspec(dllexport) int __cdecl BMIReadSOCRegister_win(HANDLE handle, A_UINT32 address, A_UINT32 *param);

__declspec(dllexport) int __cdecl BMIWriteMemory_win(HANDLE handle, A_UINT32 address, A_UCHAR *buffer, A_UINT32 length);

__declspec(dllexport) int __cdecl BMIReadMemory_win(HANDLE handle, A_UINT32 address, A_UCHAR *buffer, A_UINT32 length);

__declspec(dllexport) int __cdecl BMISetAppStart_win(HANDLE handle, A_UINT32 address);

__declspec(dllexport) int __cdecl BMIDone_win(HANDLE handle);

__declspec(dllexport) int __cdecl BMIFastDownload_win(HANDLE handle, A_UINT32 address, A_UCHAR *buffer, A_UINT32 length);

__declspec(dllexport) int __cdecl BMIExecute_win(HANDLE handle, A_UINT32 address, A_UINT32 *param);

__declspec(dllexport) int __cdecl BMITransferEepromFile_win(HANDLE handle, A_UCHAR *eeprom, A_UINT32 length);

__declspec(dllexport) void __cdecl devdrv_MyMallocStart ();

__declspec(dllexport) void __cdecl devdrv_MyMallocEnd ();

#endif //EXPORT_DLL_HEADER





