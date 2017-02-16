
//------------------------------------------------------------------------------
// File: demo_page.cpp
//
// Copyright (c) Ambarella Corporation.  All rights reserved.
//------------------------------------------------------------------------------

#include "ambas_common.h"

static struct
{
	const char *name;
	ULONG mode;
}
vout_resolution[] =
{
	{"480i (720x480)", AMBA_VIDEO_MODE_480I},
	{"576i (720x576)", AMBA_VIDEO_MODE_576I},
	{"480p (720x480)", AMBA_VIDEO_MODE_D1_NTSC},
	{"576p (720x576)", AMBA_VIDEO_MODE_D1_PAL},
	{"720p (1280x720)", AMBA_VIDEO_MODE_720P},
	{"720p50 (1280x720)", AMBA_VIDEO_MODE_720P_PAL},
	{"1080i (1920x1080)", AMBA_VIDEO_MODE_1080I},
	{"1080i50 (1920x1080)", AMBA_VIDEO_MODE_1080I_PAL},
};
#define NUM_VOUT_RESOLUTION (sizeof(vout_resolution) / sizeof(vout_resolution[0]))

static struct
{
	ULONG   width;
	ULONG   height;
}
encode_resolution[] =
{
	{1920, 1080},
	{1440, 1080},

	{1280, 1024},
	{1280, 960},
	{1280, 720},
	{1024, 768},

	{720, 480},
	{720, 576},

	{704, 480},
	{704, 576},

	{640, 480},

	{352, 288},
	{352, 240},

	{320, 240},
};
#define NUM_ENCODE_RESOLUTION   (sizeof(encode_resolution) / sizeof(encode_resolution[0]))

static struct
{
	const char *name;
	ULONG mode;
}
vin_modes[] = 
{
	{"Auto", AMBA_VIDEO_MODE_AUTO},
	{"480i", AMBA_VIDEO_MODE_480I},
	{"576i", AMBA_VIDEO_MODE_576I},
	{"480p", AMBA_VIDEO_MODE_D1_NTSC},
	{"576p", AMBA_VIDEO_MODE_D1_PAL},
	{"720p", AMBA_VIDEO_MODE_720P},
	{"1080p", AMBA_VIDEO_MODE_1080P},
	{"640x480", AMBA_VIDEO_MODE_VGA},
	{"800x600", AMBA_VIDEO_MODE_SVGA},
	{"1024x768", AMBA_VIDEO_MODE_XGA},
	{"1280x960", AMBA_VIDEO_MODE_1280_960},
	{"1280x1024", AMBA_VIDEO_MODE_SXGA},
	{"1600x1200", AMBA_VIDEO_MODE_UXGA},
	{"3M 16:9",	AMBA_VIDEO_MODE_3M_16_9},
	{"4M 4:3", AMBA_VIDEO_MODE_4M_4_3},
	{"4M 16:9",	AMBA_VIDEO_MODE_4M_16_9},
	{"5M 4:3", AMBA_VIDEO_MODE_5M_4_3},
	{"5M 16:9",	AMBA_VIDEO_MODE_5M_16_9},
};
#define NUM_VIN_MODES	   (sizeof(vin_modes) / sizeof(vin_modes[0]))

static struct
{
	const char *name;
	int framerate;
}
vin_frame_rates[] = 
{
	{"Auto", AMBA_VIDEO_FPS_AUTO},
	{"29.97", AMBA_VIDEO_FPS_29_97},
	{"59.94", AMBA_VIDEO_FPS_59_94},
	{"5", AMBA_VIDEO_FPS_5},
	{"10", AMBA_VIDEO_FPS_10},
	{"15", AMBA_VIDEO_FPS_15},
	{"25", AMBA_VIDEO_FPS_25},
	{"30", AMBA_VIDEO_FPS_30},
	{"50", AMBA_VIDEO_FPS_50},
	{"60", AMBA_VIDEO_FPS_60},
};
#define NUM_VIN_FRAME_RATE  (sizeof(vin_frame_rates) / sizeof(vin_frame_rates[0]))

static struct
{
	const char *name;
	int mode;
}
mirror_mode[] = 
{
	{"Auto", 4},
	{"0", 0},
	{"1", 1},
	{"2", 2},
	{"3", 3},
};
#define NUM_MIRROR_MODE		(sizeof(mirror_mode) / sizeof(mirror_mode[0]))

CUnknown * WINAPI CAmbaEncodeFormatProp::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
	ASSERT(phr);

	CUnknown *punk = new CAmbaEncodeFormatProp(lpunk, phr);
	if (punk == NULL && phr)
	{
		*phr = E_OUTOFMEMORY;
	}
	return punk;
}

CAmbaEncodeFormatProp::CAmbaEncodeFormatProp(LPUNKNOWN lpunk, HRESULT *phr):
	inherited(NAME("RecordControlProp"), lpunk, IDD_DIALOG_ENCODE_FORMAT, IDS_PROPPAGE_ENCODE_FORMAT),
	m_pRecordControl(NULL),
	m_pFormat(NULL)
{
	ASSERT(phr);
	*phr = S_OK;
}

HRESULT CAmbaEncodeFormatProp::OnConnect(IUnknown *punk)
{
	HRESULT hr;

	if (punk == NULL)
		return E_POINTER;

	m_pRecordControl = G_pRecordControl;

//	hr = punk->QueryInterface(IID_RecordControl, (void **)&m_pRecordControl);
//	if (FAILED(hr))
//		return hr;

	hr = m_pRecordControl->GetFormat(&m_pFormat);
	if (FAILED(hr))
	{
		m_pRecordControl->Release();
		m_pRecordControl = NULL;
		return hr;
	}

	return S_OK;
}

HRESULT CAmbaEncodeFormatProp::OnDisconnect()
{
//	if (m_pRecordControl)
//	{
//		m_pRecordControl->Release();
//		m_pRecordControl = NULL;
//	}
	return S_OK;
}

HRESULT CAmbaEncodeFormatProp::OnActivate()
{
	HWND hwnd;
	ULONG i;
	int index;
	char buffer[256];
	BOOL enable;

	// sensor input mode
	index = -1;
	hwnd = ::GetDlgItem(m_hwnd, IDC_VIN_MODE);
	for (i = 0; i < NUM_VIN_MODES; i++)
	{
		::SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)vin_modes[i].name);
		if (m_pFormat->vinMode == vin_modes[i].mode)
			index = i;
	}
	::SendMessage(hwnd, CB_SETCURSEL, index, 0);

	// frame rate
	index = -1;
	hwnd = ::GetDlgItem(m_hwnd, IDC_VIN_FRAME_RATE);
	for (i = 0; i < NUM_VIN_FRAME_RATE; i++)
	{
		::SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)vin_frame_rates[i].name);
		if (m_pFormat->vinFrate == vin_frame_rates[i].framerate)
			index = i;
	}
	::SendMessage(hwnd, CB_SETCURSEL, index, 0);

	// sensor mirror pattern
	index = -1;
	hwnd = ::GetDlgItem(m_hwnd, IDC_VIN_MIRROR_PATTERN);
	for (i = 0; i < NUM_MIRROR_MODE; ++i)
	{
		::SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)mirror_mode[i].name);
		if (m_pFormat->mirrorMode.pattern == mirror_mode[i].mode)
			index = i;
	}
	::SendMessage(hwnd, CB_SETCURSEL, index, 0);

	// sensor bayer pattern
	index = -1;
	hwnd = ::GetDlgItem(m_hwnd, IDC_VIN_BAYER_PATTERN);
	for (i = 0; i < NUM_MIRROR_MODE; ++i)
	{
		::SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)mirror_mode[i].name);
		if (m_pFormat->mirrorMode.bayer_pattern == mirror_mode[i].mode)
			index = i;
	}
	::SendMessage(hwnd, CB_SETCURSEL, index, 0);

	// main stream encode type
	if (m_pFormat->main.enc_type == IAmbaRecordControl::ENC_H264) {
		::SendDlgItemMessage(m_hwnd, IDC_MAIN_H264, BM_SETCHECK, BST_CHECKED, 0);
		enable = TRUE;
	} else if (m_pFormat->main.enc_type == IAmbaRecordControl::ENC_MJPEG) {
		::SendDlgItemMessage(m_hwnd, IDC_MAIN_MJPEG, BM_SETCHECK, BST_CHECKED, 0);
		enable = TRUE;
	} else {
		::SendDlgItemMessage(m_hwnd, IDC_MAIN_NONE, BM_SETCHECK, BST_CHECKED, 0);
		enable = FALSE;
	}
	EnableWindowById(IDC_MAIN_RESOLUTION, enable);
	EnableWindowById(IDC_MAIN_FRAME_INTERVAL, enable);

	// second stream encode type
	if (m_pFormat->secondary.enc_type == IAmbaRecordControl::ENC_H264) {
		::SendDlgItemMessage(m_hwnd, IDC_SECOND_H264, BM_SETCHECK, BST_CHECKED, 0);
		enable = TRUE;
	} else if (m_pFormat->secondary.enc_type == IAmbaRecordControl::ENC_MJPEG) {
		::SendDlgItemMessage(m_hwnd, IDC_SECOND_MJPEG, BM_SETCHECK, BST_CHECKED, 0);
		enable = TRUE;
	} else {
		::SendDlgItemMessage(m_hwnd, IDC_SECOND_NONE, BM_SETCHECK, BST_CHECKED, 0);
		enable = FALSE;
	}
	EnableWindowById(IDC_SECOND_RESOLUTION, enable);
	EnableWindowById(IDC_SECOND_FRAME_INTERVAL, enable);

	// main resolution
	index = -1;
	hwnd = ::GetDlgItem(m_hwnd, IDC_MAIN_RESOLUTION);
	for (i = 0; i < NUM_ENCODE_RESOLUTION; i++) {
		::StringCchPrintf((LPSTR)buffer, sizeof(buffer), "%d. %dx%d", 
			i, encode_resolution[i].width, encode_resolution[i].height);
		::SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)buffer);
		if (m_pFormat->main.width == encode_resolution[i].width &&
			m_pFormat->main.height == encode_resolution[i].height)
			index = i;
	}
	::SendMessage(hwnd, CB_SETCURSEL, index, 0);

	// main frame interval

	// secondary resolution
	index = -1;
	hwnd = ::GetDlgItem(m_hwnd, IDC_SECOND_RESOLUTION);
	for (i = 0; i < NUM_ENCODE_RESOLUTION; i++) 
	{
		::StringCchPrintf((LPSTR)buffer, sizeof(buffer), "%d. %dx%d", 
			i, encode_resolution[i].width, encode_resolution[i].height);
		::SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)buffer);
		if (m_pFormat->secondary.width == encode_resolution[i].width &&
			m_pFormat->secondary.height == encode_resolution[i].height)
			index = i;
	}
	::SendMessage(hwnd, CB_SETCURSEL, index, 0);

	// secondary frame interval

	// vout resolution
	index = -1;
	hwnd = ::GetDlgItem(m_hwnd, IDC_VOUT_RESOLUTION);
	for (i = 0; i < NUM_VOUT_RESOLUTION; i++)
	{
		::StringCchPrintf((LPSTR)buffer, sizeof(buffer), "%d. %s",
		i, vout_resolution[i].name);
		::SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)buffer);
		if (m_pFormat->voutMode == vout_resolution[i].mode)
			index = i;
	}
	::SendMessage(hwnd, CB_SETCURSEL, index, 0);

	return S_OK;
}

HRESULT CAmbaEncodeFormatProp::OnDeactivate()
{
	::KillTimer(m_hwnd, 1);
	return S_OK;
}

HRESULT CAmbaEncodeFormatProp::OnApplyChanges()
{
	HRESULT hr;
	int result;

	hr = m_pRecordControl->SetFormat(&result);
	if (FAILED(hr))
		return hr;

	if (result != 0) {
		MessageBox(m_hwnd, "Set format failed", "Error", MB_OK | MB_ICONEXCLAMATION);
		return E_FAIL;
	}

	return S_OK;
}

INT_PTR CAmbaEncodeFormatProp::OnReceiveMessage(
	HWND hwnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	ULONG sel;

	switch (uMsg)
	{
	case WM_COMMAND:
		if (HIWORD(wParam) == CBN_SELCHANGE)
		{
			switch (LOWORD(wParam))
			{
			case IDC_VIN_MODE:
				sel = ::SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
				if (sel < NUM_VIN_MODES)
				{
					m_pFormat->vinMode = vin_modes[sel].mode;
					SetDirty();
				}
				break;

			case IDC_VIN_FRAME_RATE:
				sel = ::SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
				if (sel < NUM_VIN_FRAME_RATE)
				{
					m_pFormat->vinFrate = vin_frame_rates[sel].framerate;
					SetDirty();
				}
				break;

			case IDC_VIN_MIRROR_PATTERN:
				sel = ::SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
				if (sel < NUM_MIRROR_MODE)
				{
					m_pFormat->mirrorMode.pattern = mirror_mode[sel].mode;
					SetDirty();
				}
				break;

			case IDC_VIN_BAYER_PATTERN:
				sel = ::SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
				if (sel < NUM_MIRROR_MODE)
				{
					m_pFormat->mirrorMode.bayer_pattern = mirror_mode[sel].mode;
					SetDirty();
				}
				break;

			case IDC_MAIN_RESOLUTION:
				sel = ::SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
				if (sel < NUM_ENCODE_RESOLUTION)
				{
					m_pFormat->main.width = encode_resolution[sel].width;
					m_pFormat->main.height = encode_resolution[sel].height;
					SetDirty();
				}
				break;

			case IDC_SECOND_RESOLUTION:
				sel = ::SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
				if (sel < NUM_ENCODE_RESOLUTION)
				{
					m_pFormat->secondary.width = encode_resolution[sel].width;
					m_pFormat->secondary.height = encode_resolution[sel].height;
					SetDirty();
				}
				break;

			case IDC_VOUT_RESOLUTION:
				sel = ::SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
				if (sel < NUM_VOUT_RESOLUTION)
				{
					m_pFormat->voutMode = vout_resolution[sel].mode;
					SetDirty();
				}
				break;
			}

			return (LRESULT)1;
		}
		else if (HIWORD(wParam) ==  BN_CLICKED)
		{
			BOOL enable1, enable2;
			switch (LOWORD(wParam))
			{
			case IDC_MAIN_NONE:
				m_pFormat->main.enc_type = IAmbaRecordControl::ENC_NONE;
				break;

			case IDC_MAIN_H264:
				m_pFormat->main.enc_type = IAmbaRecordControl::ENC_H264;
				break;

			case IDC_MAIN_MJPEG:
				m_pFormat->main.enc_type = IAmbaRecordControl::ENC_MJPEG;
				break;

			case IDC_SECOND_NONE:
				m_pFormat->secondary.enc_type = IAmbaRecordControl::ENC_NONE;
				break;

			case IDC_SECOND_H264:
				m_pFormat->secondary.enc_type = IAmbaRecordControl::ENC_H264;
				break;

			case IDC_SECOND_MJPEG:
				m_pFormat->secondary.enc_type = IAmbaRecordControl::ENC_MJPEG;
				break;

			}
			enable1 = (m_pFormat->main.enc_type != IAmbaRecordControl::ENC_NONE);
			enable2 = (m_pFormat->secondary.enc_type != IAmbaRecordControl::ENC_NONE);
			EnableWindowById(IDC_MAIN_RESOLUTION, enable1);
			EnableWindowById(IDC_MAIN_FRAME_INTERVAL, enable1);
			EnableWindowById(IDC_SECOND_RESOLUTION, enable2);
			EnableWindowById(IDC_SECOND_FRAME_INTERVAL, enable2);
			SetDirty();

			return (LRESULT)1;
		}
	}

	return inherited::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
}

int CAmbaEncodeFormatProp::EditGetInt(HWND hwnd)
{
	LRESULT size;
	char buffer[256];

	size = ::SendMessage(hwnd, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);
	if (size > 0)
		return atoi((const char *)buffer);

	return 0;
}

int CAmbaEncodeFormatProp::EnableWindowById(WORD id, BOOL enable)
{
	HWND hwnd = ::GetDlgItem(m_hwnd, id);
	return ::EnableWindow(hwnd, enable);
}

//#pragma warning(disable: 4995)

void CAmbaEncodeFormatProp::SetDirty()
{
	m_bDirty = TRUE;
	if (m_pPageSite)
	{
		m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	}
}

DWORD M[] = {1, 2, 4, 8};
#define NUM_M	   (sizeof(M) / sizeof(M[0]))

static WORD param_controls_main[] =
{
	IDC_COMBO1, // M
	IDC_EDIT1,  // N
	IDC_EDIT2,  // IDR interval
	IDC_RADIO1, // CBR
	IDC_RADIO2, // VBR
	IDC_RADIO3, // Simple
	IDC_RADIO4, // Advanced
	IDC_EDIT3,  // VBRness
	IDC_EDIT4,  // Min VBR rate
	IDC_EDIT5,  // Max VBR rate
	IDC_EDIT6,  // Average bitrate
	IDC_MAIN_PROFILE_MAIN,
	IDC_MAIN_PROFILE_BASELINE,
};

static WORD param_controls_secondary[] =
{
	IDC_COMBO3, // M
	IDC_EDIT7,  // N
	IDC_EDIT8,  // IDR interval
	IDC_RADIO5, // CBR
	IDC_RADIO6, // VBR
	IDC_RADIO7, // Simple
	IDC_RADIO8, // Advanced
	IDC_EDIT9,  // VBRness
	IDC_EDIT10,  // Min VBR rate
	IDC_EDIT11,  // Max VBR rate
	IDC_EDIT12,  // Average bitrate
	IDC_SECOND_PROFILE_MAIN,
	IDC_SECOND_PROFILE_BASELINE,
};

CUnknown * WINAPI CAmbaEncodeParamProp::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
	ASSERT(phr);

	CUnknown *punk = new CAmbaEncodeParamProp(lpunk, phr);
	if (punk == NULL && phr)
	{
		*phr = E_OUTOFMEMORY;
	}
	return punk;
}

CAmbaEncodeParamProp::CAmbaEncodeParamProp(LPUNKNOWN lpunk, HRESULT *phr):
	inherited(NAME("RecordControlProp"), lpunk, IDD_DIALOG_ENCODE_PARAM, IDS_PROPPAGE_ENCODE_PARAM),
	m_pRecordControl(NULL),
	m_pFormat(NULL),
	m_pParam(NULL)
{
}

HRESULT CAmbaEncodeParamProp::OnConnect(IUnknown *punk)
{
	HRESULT hr;

	if (punk == NULL)
		return E_POINTER;

	m_pRecordControl = G_pRecordControl;

//	hr = punk->QueryInterface(IID_RecordControl, (void **)&m_pRecordControl);
//	if (FAILED(hr))
//		return hr;

	hr = m_pRecordControl->GetFormat(&m_pFormat);
	if (FAILED(hr))
	{
		m_pRecordControl->Release();
		m_pRecordControl = NULL;
	}

	hr = m_pRecordControl->GetParam(&m_pParam);
	if (FAILED(hr))
	{
		m_pRecordControl->Release();
		m_pRecordControl = NULL;
	}

	return S_OK;
}

HRESULT CAmbaEncodeParamProp::OnDisconnect()
{
//	if (m_pRecordControl)
//	{
//		m_pRecordControl->Release();
//		m_pRecordControl = NULL;
//	}
	return S_OK;
}

HRESULT CAmbaEncodeParamProp::OnActivate()
{
	char buffer[256];
	DWORD numControls;
	BOOL enable;

	// main H.264
	enable = m_pFormat->main.enc_type == IAmbaRecordControl::ENC_H264;
	numControls = sizeof(param_controls_main) / sizeof(param_controls_main[0]);
	SetConfig(&m_pParam->main, param_controls_main, numControls, enable);

	// secondary H.264
	enable = m_pFormat->secondary.enc_type == IAmbaRecordControl::ENC_H264;
	numControls = sizeof(param_controls_secondary) / sizeof(param_controls_secondary[0]);
	SetConfig(&m_pParam->secondary, param_controls_secondary, numControls, enable);

	// jpeg quality
	::StringCchPrintf((LPSTR)buffer, sizeof(buffer), "%d", m_pParam->mjpeg_quality);
	::SendDlgItemMessage(m_hwnd, IDC_EDIT13, WM_SETTEXT, 0, (LPARAM)buffer);

	return S_OK;
}

HRESULT CAmbaEncodeParamProp::OnDeactivate()
{
	return S_OK;
}

HRESULT CAmbaEncodeParamProp::OnApplyChanges()
{
	HRESULT hr;
	int result;

	hr = m_pRecordControl->SetParam(&result);
	if (FAILED(hr))
		return hr;

	if (result != 0) {
		MessageBox(m_hwnd, "Set parameters failed", "Error", MB_OK | MB_ICONEXCLAMATION);
		return E_FAIL;
	}

	return S_OK;
}

INT_PTR CAmbaEncodeParamProp::OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ULONG sel;
	int value;

	switch (uMsg)
	{
	case WM_COMMAND:
		switch (HIWORD(wParam))
		{
		case CBN_SELCHANGE:
			switch (LOWORD(wParam))
			{
			case IDC_COMBO1:
			case IDC_COMBO3:
				sel = ::SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
				if (LOWORD(wParam) == IDC_COMBO1)
					m_pParam->main.M = (BYTE)M[sel];
				else
					m_pParam->secondary.M = (BYTE)M[sel];
				SetDirty();
				break;

			default:
				break;
			}
			break;

		case EN_CHANGE:
			value = EditGetInt((HWND)lParam);
			switch (LOWORD(wParam))
			{
			case IDC_EDIT1: m_pParam->main.N = value; break;
			case IDC_EDIT2: m_pParam->main.idr_interval = value; break;
			case IDC_EDIT3: m_pParam->main.vbr_ness = value; break;
			case IDC_EDIT4: m_pParam->main.min_vbr_rate_factor = value; break;
			case IDC_EDIT5: m_pParam->main.max_vbr_rate_factor = value; break;
			case IDC_EDIT6: m_pParam->main.average_bitrate = value; break;

			case IDC_EDIT7: m_pParam->secondary.N = value; break;
			case IDC_EDIT8: m_pParam->secondary.idr_interval = value; break;
			case IDC_EDIT9: m_pParam->secondary.vbr_ness = value; break;
			case IDC_EDIT10: m_pParam->secondary.min_vbr_rate_factor = value; break;
			case IDC_EDIT11: m_pParam->secondary.max_vbr_rate_factor = value; break;
			case IDC_EDIT12: m_pParam->secondary.average_bitrate = value; break;
			}
			SetDirty();
			break;

		case BN_CLICKED:
			switch (LOWORD(wParam))
			{
			case IDC_RADIO1: m_pParam->main.bitrate_control = IAmbaRecordControl::CBR; break;
			case IDC_RADIO2: m_pParam->main.bitrate_control = IAmbaRecordControl::VBR; break;
			case IDC_RADIO3: m_pParam->main.gop_model = IAmbaRecordControl::GOP_SIMPLE; break;
			case IDC_RADIO4: m_pParam->main.gop_model = IAmbaRecordControl::GOP_ADVANCED; break;
			case IDC_MAIN_PROFILE_MAIN: m_pParam->main.profile = IAmbaRecordControl::MAIN; break;
			case IDC_MAIN_PROFILE_BASELINE: m_pParam->main.profile = IAmbaRecordControl::BASELINE; break;

			case IDC_RADIO5: m_pParam->secondary.bitrate_control = IAmbaRecordControl::CBR; break;
			case IDC_RADIO6: m_pParam->secondary.bitrate_control = IAmbaRecordControl::VBR; break;
			case IDC_RADIO7: m_pParam->secondary.gop_model = IAmbaRecordControl::GOP_SIMPLE; break;
			case IDC_RADIO8: m_pParam->secondary.gop_model = IAmbaRecordControl::GOP_ADVANCED; break;
			case IDC_SECOND_PROFILE_MAIN: m_pParam->secondary.profile = IAmbaRecordControl::MAIN; break;
			case IDC_SECOND_PROFILE_BASELINE: m_pParam->secondary.profile = IAmbaRecordControl::BASELINE; break;

//			case IDC_BUTTON1: SetLowDelay(&m_pParam->main, param_controls_main); break;
//			case IDC_BUTTON2: SetLowDelay(&m_pParam->secondary, param_controls_secondary); break;
			}
			SetDirty();
			return inherited::OnReceiveMessage(hwnd, uMsg, wParam, lParam);

		default:
			return inherited::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
		}
		return (LRESULT)1;

	default:
		break;
	}

	return inherited::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
}

void CAmbaEncodeParamProp::SetConfig(IAmbaRecordControl::H264_PARAM *pParam, 
	WORD *pControls, DWORD numControls, BOOL enable)
{
	HWND hwnd;
	DWORD i;
	int index;
	char buffer[256];

	// M
	index = -1;
	hwnd = ::GetDlgItem(m_hwnd, pControls[0]);
	for (i = 0; i < NUM_M; i++)
	{
		::StringCchPrintf((LPSTR)buffer, sizeof(buffer), "%d", M[i]);
		::SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)buffer);
		if (pParam->M == M[i])
			index = i;
	}
	::SendMessage(hwnd, CB_SETCURSEL, index, 0);

	// N
	hwnd = ::GetDlgItem(m_hwnd, pControls[1]);
	::StringCchPrintf((LPSTR)buffer, sizeof(buffer), "%d", pParam->N);
	::SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buffer);

	// IDR interval
	hwnd = ::GetDlgItem(m_hwnd, pControls[2]);
	::StringCchPrintf((LPSTR)buffer, sizeof(buffer), "%d", pParam->idr_interval);
	::SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buffer);
	
	// bitrate control
	if (pParam->bitrate_control == IAmbaRecordControl::CBR)
		hwnd = ::GetDlgItem(m_hwnd, pControls[3]);
	else
		hwnd = ::GetDlgItem(m_hwnd, pControls[4]);

	::SendMessage(hwnd, BM_SETCHECK, BST_CHECKED, 0);

	// gop model
	if (pParam->gop_model == IAmbaRecordControl::GOP_SIMPLE)
		hwnd = ::GetDlgItem(m_hwnd, pControls[5]);
	else
		hwnd = ::GetDlgItem(m_hwnd, pControls[6]);

	::SendMessage(hwnd, BM_SETCHECK, BST_CHECKED, 0);

	//VBRness
	hwnd = ::GetDlgItem(m_hwnd, pControls[7]);
	::StringCchPrintf((LPSTR)buffer, sizeof(buffer), "%d", pParam->vbr_ness);
	::SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buffer);

	// Min VBR rate factor
	hwnd = ::GetDlgItem(m_hwnd, pControls[8]);
	::StringCchPrintf((LPSTR)buffer, sizeof(buffer), "%d", pParam->min_vbr_rate_factor);
	::SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buffer);

	// Max VBR rate factor
	hwnd = ::GetDlgItem(m_hwnd, pControls[9]);
	::StringCchPrintf((LPSTR)buffer, sizeof(buffer), "%d", pParam->max_vbr_rate_factor);
	::SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buffer);

	// Average bitrate
	hwnd = ::GetDlgItem(m_hwnd, pControls[10]);
	::StringCchPrintf((LPSTR)buffer, sizeof(buffer), "%d", pParam->average_bitrate);
	::SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buffer);

	// Profile
	if (pParam->profile == IAmbaRecordControl::MAIN)
		hwnd = ::GetDlgItem(m_hwnd, pControls[11]);
	else
		hwnd = ::GetDlgItem(m_hwnd, pControls[12]);

	::SendMessage(hwnd, BM_SETCHECK, BST_CHECKED, 0);

	if (!enable)
		for (i = 0; i < numControls; i++)
		{
			hwnd = ::GetDlgItem(m_hwnd, pControls[i]);
			::EnableWindow(hwnd, enable);
		}
}

void CAmbaEncodeParamProp::SetLowDelay(IAmbaRecordControl::H264_PARAM *pParam, WORD *pControls)
{
	HWND hwnd;

	hwnd = ::GetDlgItem(m_hwnd, pControls[0]);
	::SendMessage(hwnd, CB_SETCURSEL, 0, 0);
	pParam->M = 1;

	hwnd = ::GetDlgItem(m_hwnd, pControls[5]);
	::SendMessage(hwnd, BM_CLICK, 0, 0);
	pParam->gop_model = IAmbaRecordControl::GOP_SIMPLE;
}

void CAmbaEncodeParamProp::SetDirty()
{
	m_bDirty = TRUE;
	if (m_pPageSite)
	{
		m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	}
}

int CAmbaEncodeParamProp::EditGetInt(HWND hwnd)
{
	LRESULT size;
	char buffer[256];

	size = ::SendMessage(hwnd, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);
	if (size > 0)
		return atoi((const char *)buffer);

	return 0;
}

enum amba_lens_ID {
	AMBA_LENS_FULL_MANUAL = 0,
	AMBA_LENS_DC_IRIS_FIXED_FOCAL = (0 | (1<<16)),
	AMBA_LENS_JCD_661 = (2 | (1<<17)),
	AMBA_LENS_RICOM_NL3XZD = (3 | (1<<17)),
	AMBA_LENS_TAMRON_187YC = (4 | (1<<16) | (1<<17) | (1<<18)),
};
static struct
{
	const char *name;
	unsigned int lens_id;
}
lens_types[] = 
{
	{"Full manual",		AMBA_LENS_FULL_MANUAL},
	{"DC iris fixed focal",	 AMBA_LENS_DC_IRIS_FIXED_FOCAL},
	{"JCD-661",		AMBA_LENS_JCD_661},
	{"Ricom-NL3XZD",		AMBA_LENS_RICOM_NL3XZD},
	{"Tamron-187YC",		AMBA_LENS_TAMRON_187YC},
};
#define NUM_LENS_TYPES	   (sizeof(lens_types) / sizeof(lens_types[0]))

CUnknown * WINAPI CAmbaImgParamProp::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
	ASSERT(phr);

	CUnknown *punk = new CAmbaImgParamProp(lpunk, phr);
	if (punk == NULL && phr)
	{
		*phr = E_OUTOFMEMORY;
	}
	return punk;
}

CAmbaImgParamProp::CAmbaImgParamProp(LPUNKNOWN lpunk, HRESULT *phr):
	inherited(NAME("RecordControlProp"), lpunk, IDD_DIALOG_IMG_PARAM, IDS_PROPPAGE_IMG_PARAM),
	m_pRecordControl(NULL),
	m_pImgParam(NULL)
{
}

HRESULT CAmbaImgParamProp::OnConnect(IUnknown *punk)
{
	HRESULT hr;

	if (punk == NULL)
		return E_POINTER;

	m_pRecordControl = G_pRecordControl;

	hr = m_pRecordControl->GetImgParam(&m_pImgParam);
	if (FAILED(hr))
	{
		m_pRecordControl->Release();
		m_pRecordControl = NULL;
	}

	return S_OK;
}

HRESULT CAmbaImgParamProp::OnDisconnect()
{
//	if (m_pRecordControl)
//	{
//		m_pRecordControl->Release();
//		m_pRecordControl = NULL;
//	}
	return S_OK;
}

HRESULT CAmbaImgParamProp::OnActivate()
{
	HWND hwnd;
	ULONG i;
	int index = -1;

	// lense type
	hwnd = ::GetDlgItem(m_hwnd, IDC_COMBO_LENS_TYPE);
	for (i = 0; i < NUM_LENS_TYPES; i++)
	{
		::SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)lens_types[i].name);
		if (m_pImgParam->lens_type == lens_types[i].lens_id)
			index = i;
	}
	::SendMessage(hwnd, CB_SETCURSEL, index, 0);

	 // anti-flicker mode
	if (m_pImgParam->anti_flicker_mode == IAmbaRecordControl::ANTI_FLICKER_50HZ)
		hwnd = ::GetDlgItem(m_hwnd, IDC_RADIO_ANTI_FLICKER_50Hz);
	else
		hwnd = ::GetDlgItem(m_hwnd, IDC_RADIO_ANTI_FLICKER_60Hz);
	::SendMessage(hwnd, BM_SETCHECK, BST_CHECKED, 0);
	
	if (m_pImgParam->black_white_mode == IAmbaRecordControl::MODE_OFF)
		hwnd = ::GetDlgItem(m_hwnd, IDC_RADIO_BLACK_WHITE_OFF);
	else if (m_pImgParam->black_white_mode == IAmbaRecordControl::MODE_ON)
		hwnd = ::GetDlgItem(m_hwnd, IDC_RADIO_BLACK_WHITE_ON);
	else
		hwnd = ::GetDlgItem(m_hwnd, IDC_RADIO_BLACK_WHITE_AUTO);
	::SendMessage(hwnd, BM_SETCHECK, BST_CHECKED, 0);
	
	if (m_pImgParam->slow_shutter_mode == IAmbaRecordControl::MODE_OFF)
		hwnd = ::GetDlgItem(m_hwnd, IDC_RADIO_SLOW_SHUTTER_OFF);
	else if (m_pImgParam->slow_shutter_mode== IAmbaRecordControl::MODE_ON)
		hwnd = ::GetDlgItem(m_hwnd, IDC_RADIO_SLOW_SHUTTER_ON);
	else
		hwnd = ::GetDlgItem(m_hwnd, IDC_RADIO_SLOW_SHUTTER_AUTO);
	::SendMessage(hwnd, BM_SETCHECK, BST_CHECKED, 0);

	if (m_pImgParam->aaa_enable_mode == IAmbaRecordControl::MODE_OFF)
		hwnd = ::GetDlgItem(m_hwnd, IDC_RADIO_3A_ENABLE_OFF);
	else
		hwnd = ::GetDlgItem(m_hwnd, IDC_RADIO_3A_ENABLE_ON);
	::SendMessage(hwnd, BM_SETCHECK, BST_CHECKED, 0);

	return S_OK;
}

HRESULT CAmbaImgParamProp::OnDeactivate()
{
	return S_OK;
}

HRESULT CAmbaImgParamProp::OnApplyChanges()
{
	HRESULT hr;
	int result;

	hr = m_pRecordControl->SetImgParam(&result);
	if (FAILED(hr))
		return hr;

	if (result != 0) {
		MessageBox(m_hwnd, "Set image parameters failed", "Error", MB_OK | MB_ICONEXCLAMATION);
		return E_FAIL;
	}

	return S_OK;
}

INT_PTR CAmbaImgParamProp::OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ULONG sel;

	switch (uMsg)
	{
	case WM_COMMAND:
		switch (HIWORD(wParam))
		{
		case CBN_SELCHANGE:
			switch (LOWORD(wParam))
			{
			case IDC_COMBO_LENS_TYPE:
				sel = ::SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
				m_pImgParam->lens_type = lens_types[sel].lens_id;
				SetDirty();
				break;

			default:
				break;
			}
			break;

		case BN_CLICKED:
			switch (LOWORD(wParam))
			{
			case IDC_RADIO_ANTI_FLICKER_60Hz: m_pImgParam->anti_flicker_mode = IAmbaRecordControl::ANTI_FLICKER_60HZ; break;
			case IDC_RADIO_ANTI_FLICKER_50Hz: m_pImgParam->anti_flicker_mode = IAmbaRecordControl::ANTI_FLICKER_50HZ; break; 
			case IDC_RADIO_BLACK_WHITE_OFF: m_pImgParam->black_white_mode = IAmbaRecordControl::MODE_OFF; break; 
			case IDC_RADIO_BLACK_WHITE_ON: m_pImgParam->black_white_mode = IAmbaRecordControl::MODE_ON; break;
			case IDC_RADIO_BLACK_WHITE_AUTO: m_pImgParam->black_white_mode = IAmbaRecordControl::MODE_AUTO; break; 
			case IDC_RADIO_SLOW_SHUTTER_OFF: m_pImgParam->slow_shutter_mode = IAmbaRecordControl::MODE_OFF; break; 
			case IDC_RADIO_SLOW_SHUTTER_ON: m_pImgParam->slow_shutter_mode = IAmbaRecordControl::MODE_ON; break;
			case IDC_RADIO_SLOW_SHUTTER_AUTO: m_pImgParam->slow_shutter_mode = IAmbaRecordControl::MODE_AUTO; break; 
			case IDC_RADIO_3A_ENABLE_OFF: m_pImgParam->aaa_enable_mode = IAmbaRecordControl::MODE_OFF; break;
			case IDC_RADIO_3A_ENABLE_ON: m_pImgParam->aaa_enable_mode = IAmbaRecordControl::MODE_ON; break;
			}
			SetDirty();
			return inherited::OnReceiveMessage(hwnd, uMsg, wParam, lParam);

		default:
			return inherited::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
		}
		return (LRESULT)1;

	default:
		break;
	}

	return inherited::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
}

void CAmbaImgParamProp::SetDirty()
{
	m_bDirty = TRUE;
	if (m_pPageSite)
	{
		m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	}
}

int CAmbaImgParamProp::EditGetInt(HWND hwnd)
{
	LRESULT size;
	char buffer[256];

	size = ::SendMessage(hwnd, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);
	if (size > 0)
		return atoi((const char *)buffer);

	return 0;
}

CUnknown * WINAPI CAmbaMdParamProp::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
	ASSERT(phr);

	CUnknown *punk = new CAmbaMdParamProp(lpunk, phr);
	if (punk == NULL && phr)
	{
		*phr = E_OUTOFMEMORY;
	}
	return punk;
}

CAmbaMdParamProp::CAmbaMdParamProp(LPUNKNOWN lpunk, HRESULT *phr):
	inherited(NAME("RecordControlProp"), lpunk, IDD_DIALOG_MD_PARAM, IDS_PROPPAGE_MD_PARAM),
	m_pRecordControl(NULL),
	m_pMdParam(NULL)
{
}

HRESULT CAmbaMdParamProp::OnConnect(IUnknown *punk)
{
	HRESULT hr;

	if (punk == NULL)
		return E_POINTER;

	m_pRecordControl = G_pRecordControl;

	hr = m_pRecordControl->GetMdParam(&m_pMdParam);
	if (FAILED(hr))
	{
		m_pRecordControl->Release();
		m_pRecordControl = NULL;
	}

	return S_OK;
}

HRESULT CAmbaMdParamProp::OnDisconnect()
{
//	if (m_pRecordControl)
//	{
//		m_pRecordControl->Release();
//		m_pRecordControl = NULL;
//	}
	return S_OK;
}

HRESULT CAmbaMdParamProp::OnActivate()
{
	HWND hwnd;
	char buffer[64];

	// ROI info
	hwnd = ::GetDlgItem(m_hwnd, IDC_EDIT_ROI1_LU_X);
	::StringCchPrintf((LPSTR)buffer, sizeof(buffer), "%d", m_pMdParam->x_lu[0]);
	::SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buffer);  
	hwnd = ::GetDlgItem(m_hwnd, IDC_EDIT_ROI1_LU_Y);
	::StringCchPrintf((LPSTR)buffer, sizeof(buffer), "%d", m_pMdParam->y_lu[0]);
	::SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buffer);  
	hwnd = ::GetDlgItem(m_hwnd, IDC_EDIT_ROI1_RD_X);
	::StringCchPrintf((LPSTR)buffer, sizeof(buffer), "%d", m_pMdParam->x_rd[0]);
	::SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buffer); 
	hwnd = ::GetDlgItem(m_hwnd, IDC_EDIT_ROI1_RD_Y);
	::StringCchPrintf((LPSTR)buffer, sizeof(buffer), "%d", m_pMdParam->y_rd[0]);
	::SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buffer);
	hwnd = ::GetDlgItem(m_hwnd, IDC_EDIT_ROI1_THR);
	::StringCchPrintf((LPSTR)buffer, sizeof(buffer), "%d", m_pMdParam->threshold[0]);
	::SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buffer); 
	hwnd = ::GetDlgItem(m_hwnd, IDC_EDIT_ROI1_SENS);
	::StringCchPrintf((LPSTR)buffer, sizeof(buffer), "%d", m_pMdParam->sensitivity[0]);
	::SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buffer);
 	if (m_pMdParam->valid[0] == IAmbaRecordControl::MODE_ON)
		hwnd = ::GetDlgItem(m_hwnd, IDC_RADIO_ROI1_ON);
	else
		hwnd = ::GetDlgItem(m_hwnd, IDC_RADIO_ROI1_OFF);
	::SendMessage(hwnd, BM_SETCHECK, BST_CHECKED, 0);
	
	hwnd = ::GetDlgItem(m_hwnd, IDC_EDIT_ROI2_LU_X);
	::StringCchPrintf((LPSTR)buffer, sizeof(buffer), "%d", m_pMdParam->x_lu[1]);
	::SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buffer);  
	hwnd = ::GetDlgItem(m_hwnd, IDC_EDIT_ROI2_LU_Y);
	::StringCchPrintf((LPSTR)buffer, sizeof(buffer), "%d", m_pMdParam->y_lu[1]);
	::SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buffer);  
	hwnd = ::GetDlgItem(m_hwnd, IDC_EDIT_ROI2_RD_X);
	::StringCchPrintf((LPSTR)buffer, sizeof(buffer), "%d", m_pMdParam->x_rd[1]);
	::SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buffer); 
	hwnd = ::GetDlgItem(m_hwnd, IDC_EDIT_ROI2_RD_Y);
	::StringCchPrintf((LPSTR)buffer, sizeof(buffer), "%d", m_pMdParam->y_rd[1]);
	::SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buffer);
	hwnd = ::GetDlgItem(m_hwnd, IDC_EDIT_ROI2_THR);
	::StringCchPrintf((LPSTR)buffer, sizeof(buffer), "%d", m_pMdParam->threshold[1]);
	::SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buffer); 
	hwnd = ::GetDlgItem(m_hwnd, IDC_EDIT_ROI2_SENS);
	::StringCchPrintf((LPSTR)buffer, sizeof(buffer), "%d", m_pMdParam->sensitivity[1]);
	::SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buffer);
 	if (m_pMdParam->valid[1] == IAmbaRecordControl::MODE_ON)
		hwnd = ::GetDlgItem(m_hwnd, IDC_RADIO_ROI2_ON);
	else
		hwnd = ::GetDlgItem(m_hwnd, IDC_RADIO_ROI2_OFF);
	::SendMessage(hwnd, BM_SETCHECK, BST_CHECKED, 0);
	
	hwnd = ::GetDlgItem(m_hwnd, IDC_EDIT_ROI3_LU_X);
	::StringCchPrintf((LPSTR)buffer, sizeof(buffer), "%d", m_pMdParam->x_lu[2]);
	::SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buffer);  
	hwnd = ::GetDlgItem(m_hwnd, IDC_EDIT_ROI3_LU_Y);
	::StringCchPrintf((LPSTR)buffer, sizeof(buffer), "%d", m_pMdParam->y_lu[2]);
	::SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buffer);  
	hwnd = ::GetDlgItem(m_hwnd, IDC_EDIT_ROI3_RD_X);
	::StringCchPrintf((LPSTR)buffer, sizeof(buffer), "%d", m_pMdParam->x_rd[2]);
	::SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buffer); 
	hwnd = ::GetDlgItem(m_hwnd, IDC_EDIT_ROI3_RD_Y);
	::StringCchPrintf((LPSTR)buffer, sizeof(buffer), "%d", m_pMdParam->y_rd[2]);
	::SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buffer);
	hwnd = ::GetDlgItem(m_hwnd, IDC_EDIT_ROI3_THR);
	::StringCchPrintf((LPSTR)buffer, sizeof(buffer), "%d", m_pMdParam->threshold[2]);
	::SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buffer); 
	hwnd = ::GetDlgItem(m_hwnd, IDC_EDIT_ROI3_SENS);
	::StringCchPrintf((LPSTR)buffer, sizeof(buffer), "%d", m_pMdParam->sensitivity[2]);
	::SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buffer);
 	if (m_pMdParam->valid[2] == IAmbaRecordControl::MODE_ON)
		hwnd = ::GetDlgItem(m_hwnd, IDC_RADIO_ROI3_ON);
	else
		hwnd = ::GetDlgItem(m_hwnd, IDC_RADIO_ROI3_OFF);
	::SendMessage(hwnd, BM_SETCHECK, BST_CHECKED, 0);
	
	hwnd = ::GetDlgItem(m_hwnd, IDC_EDIT_ROI4_LU_X);
	::StringCchPrintf((LPSTR)buffer, sizeof(buffer), "%d", m_pMdParam->x_lu[3]);
	::SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buffer);  
	hwnd = ::GetDlgItem(m_hwnd, IDC_EDIT_ROI4_LU_Y);
	::StringCchPrintf((LPSTR)buffer, sizeof(buffer), "%d", m_pMdParam->y_lu[3]);
	::SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buffer);  
	hwnd = ::GetDlgItem(m_hwnd, IDC_EDIT_ROI4_RD_X);
	::StringCchPrintf((LPSTR)buffer, sizeof(buffer), "%d", m_pMdParam->x_rd[3]);
	::SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buffer); 
	hwnd = ::GetDlgItem(m_hwnd, IDC_EDIT_ROI4_RD_Y);
	::StringCchPrintf((LPSTR)buffer, sizeof(buffer), "%d", m_pMdParam->y_rd[3]);
	::SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buffer);
	hwnd = ::GetDlgItem(m_hwnd, IDC_EDIT_ROI4_THR);
	::StringCchPrintf((LPSTR)buffer, sizeof(buffer), "%d", m_pMdParam->threshold[3]);
	::SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buffer); 
	hwnd = ::GetDlgItem(m_hwnd, IDC_EDIT_ROI4_SENS);
	::StringCchPrintf((LPSTR)buffer, sizeof(buffer), "%d", m_pMdParam->sensitivity[3]);
	::SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM)buffer);
 	if (m_pMdParam->valid[3] == IAmbaRecordControl::MODE_ON)
		hwnd = ::GetDlgItem(m_hwnd, IDC_RADIO_ROI4_ON);
	else
		hwnd = ::GetDlgItem(m_hwnd, IDC_RADIO_ROI4_OFF);
	::SendMessage(hwnd, BM_SETCHECK, BST_CHECKED, 0);
	
	 // motion data source
	if (m_pMdParam->data_src == IAmbaRecordControl::MD_SRC_AAA)
		hwnd = ::GetDlgItem(m_hwnd, IDC_RADIO_MD_SRC_AAA);
	else
		hwnd = ::GetDlgItem(m_hwnd, IDC_RADIO_MD_SRC_PREV);
	::SendMessage(hwnd, BM_SETCHECK, BST_CHECKED, 0);

	return S_OK;
}

HRESULT CAmbaMdParamProp::OnDeactivate()
{
	return S_OK;
}

HRESULT CAmbaMdParamProp::OnApplyChanges()
{
	HRESULT hr;
	int result;

	hr = m_pRecordControl->SetMdParam(&result);
	if (FAILED(hr))
		return hr;

	if (result != 0) {
		MessageBox(m_hwnd, "Set MD parameters failed", "Error", MB_OK | MB_ICONEXCLAMATION);
		return E_FAIL;
	}

	return S_OK;
}

INT_PTR CAmbaMdParamProp::OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int value;

	switch (uMsg)
	{
	case WM_COMMAND:
		switch (HIWORD(wParam))
		{
			
	case EN_CHANGE:
			value = EditGetInt((HWND)lParam);
			switch (LOWORD(wParam))
			{
			case IDC_EDIT_ROI1_LU_X: m_pMdParam->x_lu[0] = value; break;
			case IDC_EDIT_ROI1_LU_Y: m_pMdParam->y_lu[0] = value; break;
			case IDC_EDIT_ROI1_RD_X: m_pMdParam->x_rd[0] = value; break;
			case IDC_EDIT_ROI1_RD_Y: m_pMdParam->y_rd[0] = value; break;
			case IDC_EDIT_ROI1_THESH: m_pMdParam->threshold[0] = value; break;
			case IDC_EDIT_ROI1_SENS: m_pMdParam->sensitivity[0] = value; break;
			
			case IDC_EDIT_ROI2_LU_X: m_pMdParam->x_lu[1] = value; break;
			case IDC_EDIT_ROI2_LU_Y: m_pMdParam->y_lu[1] = value; break;
			case IDC_EDIT_ROI2_RD_X: m_pMdParam->x_rd[1] = value; break;
			case IDC_EDIT_ROI2_RD_Y: m_pMdParam->y_rd[1] = value; break;
			case IDC_EDIT_ROI2_THESH: m_pMdParam->threshold[1] = value; break;
			case IDC_EDIT_ROI2_SENS: m_pMdParam->sensitivity[1] = value; break;
			
			case IDC_EDIT_ROI3_LU_X: m_pMdParam->x_lu[2] = value; break;
			case IDC_EDIT_ROI3_LU_Y: m_pMdParam->y_lu[2] = value; break;
			case IDC_EDIT_ROI3_RD_X: m_pMdParam->x_rd[2] = value; break;
			case IDC_EDIT_ROI3_RD_Y: m_pMdParam->y_rd[2] = value; break;
			case IDC_EDIT_ROI3_THESH: m_pMdParam->threshold[2] = value; break;
			case IDC_EDIT_ROI3_SENS: m_pMdParam->sensitivity[2] = value; break;
			
			case IDC_EDIT_ROI4_LU_X: m_pMdParam->x_lu[3] = value; break;
			case IDC_EDIT_ROI4_LU_Y: m_pMdParam->y_lu[3] = value; break;
			case IDC_EDIT_ROI4_RD_X: m_pMdParam->x_rd[3] = value; break;
			case IDC_EDIT_ROI4_RD_Y: m_pMdParam->y_rd[3] = value; break;
			case IDC_EDIT_ROI4_THESH: m_pMdParam->threshold[3] = value; break;
			case IDC_EDIT_ROI4_SENS: m_pMdParam->sensitivity[3] = value; break;
			}
			SetDirty();
			break;
			
		case BN_CLICKED:
			switch (LOWORD(wParam))
			{
			case IDC_RADIO_ROI1_ON: m_pMdParam->valid[0] =
				IAmbaRecordControl::MODE_ON; 
				break;
			case IDC_RADIO_ROI1_OFF: m_pMdParam->valid[0] =
				IAmbaRecordControl::MODE_OFF; 
				break;
			case IDC_RADIO_ROI2_ON: m_pMdParam->valid[1] =
				IAmbaRecordControl::MODE_ON; 
				break;
			case IDC_RADIO_ROI2_OFF: m_pMdParam->valid[1] =
				IAmbaRecordControl::MODE_OFF; 
				break;
			case IDC_RADIO_ROI3_ON: m_pMdParam->valid[2] =
				IAmbaRecordControl::MODE_ON; 
				break;
			case IDC_RADIO_ROI3_OFF: m_pMdParam->valid[2] =
				IAmbaRecordControl::MODE_OFF; 
				break;
			case IDC_RADIO_ROI4_ON: m_pMdParam->valid[3] =
				IAmbaRecordControl::MODE_ON; 
				break;
			case IDC_RADIO_ROI4_OFF: m_pMdParam->valid[3] =
				IAmbaRecordControl::MODE_OFF; 
				break;
			case IDC_RADIO_MD_SRC_AAA: m_pMdParam->data_src = 
				IAmbaRecordControl::MD_SRC_AAA; 
				break;
			case IDC_RADIO_MD_SRC_PREV: m_pMdParam->data_src = 
				IAmbaRecordControl::MD_SRC_PREV; 
				break;  
			}
			SetDirty();
			return inherited::OnReceiveMessage(hwnd, uMsg, wParam, lParam);

		default:
			return inherited::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
		}
		return (LRESULT)1;

	default:
		break;
	}

	return inherited::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
}

void CAmbaMdParamProp::SetDirty()
{
	m_bDirty = TRUE;
	if (m_pPageSite)
	{
		m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	}
}

int CAmbaMdParamProp::EditGetInt(HWND hwnd)
{
	LRESULT size;
	char buffer[256];

	size = ::SendMessage(hwnd, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);
	if (size > 0)
		return atoi((const char *)buffer);

	return 0;
}

CUnknown * WINAPI CAmbaAboutProp::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
	ASSERT(phr);

	CUnknown *punk = new CAmbaAboutProp(lpunk, phr);
	if (punk == NULL && phr)
	{
		*phr = E_OUTOFMEMORY;
	}
	return punk;
}

CAmbaAboutProp::CAmbaAboutProp(LPUNKNOWN lpunk, HRESULT *phr):
	inherited(NAME("RecordControlProp"), lpunk, IDD_DIALOG_ABOUT, IDS_PROPPAGE_ABOUT)
{
}

CUnknown * WINAPI CAmbaMuxerParamProp::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
	ASSERT(phr);

	CUnknown *punk = new CAmbaMuxerParamProp(lpunk, phr);
	if (punk == NULL && phr)
	{
		*phr = E_OUTOFMEMORY;
	}
	return punk;
}

CAmbaMuxerParamProp::CAmbaMuxerParamProp(LPUNKNOWN lpunk, HRESULT *phr):
inherited(NAME("RecordControlProp"), lpunk, IDD_DIALOG_MUXER_PARAM,IDS_PROPPAGE_MUXER_PARAM),
	m_pRecordControl(NULL),
	m_pFormat(NULL)
{
	ASSERT(phr);
	*phr = S_OK;
}

HRESULT CAmbaMuxerParamProp::OnConnect(IUnknown *punk)
{
	HRESULT hr;

	if (punk == NULL)
		return E_POINTER;

	m_pRecordControl = G_pRecordControl;

//	hr = punk->QueryInterface(IID_RecordControl, (void **)&m_pRecordControl);
//	if (FAILED(hr))
//		return hr;

	hr = m_pRecordControl->GetFormat(&m_pFormat);
	if (FAILED(hr))
	{
		m_pRecordControl->Release();
		m_pRecordControl = NULL;
		return hr;
	}

	return S_OK;
}

HRESULT CAmbaMuxerParamProp::OnDisconnect()
{

	return S_OK;
}

HRESULT CAmbaMuxerParamProp::OnActivate()
{
	//mp4muxer
	char *filename = strrchr(G_pMuxerParam->MuxerFilename, '.');
	int file_len;
	if (filename != NULL)
	{
		file_len = filename - G_pMuxerParam->MuxerFilename;
	}
	else
	{
		file_len = strlen(G_pMuxerParam->MuxerFilename);
	}
	if (m_pFormat->main.enc_type == IAmbaRecordControl::ENC_MJPEG)
	{

		::memcpy(G_pMuxerParam->MuxerFilename + file_len,".mjpeg", strlen(".mjpeg")+1);
	}
	else if (m_pFormat->main.enc_type == IAmbaRecordControl::ENC_H264 && 
		(G_pMuxerParam->MuxerType == 0 || G_pMuxerParam->MuxerType == 3 ))
	{
		::memcpy(G_pMuxerParam->MuxerFilename + file_len,".264", strlen(".264")+1);
	}
	::SetDlgItemText(m_hwnd,IDC_EDIT3,(LPTSTR)G_pMuxerParam->MuxerFilename);

	char tmp_maxfileszie[256];
	_itoa( G_pMuxerParam->MaxFileSize, tmp_maxfileszie, 10 );

	::SetDlgItemText(m_hwnd,IDC_EDIT4,(LPTSTR)tmp_maxfileszie);

	EnableWindowById(IDC_RADIO4, TRUE);
	if(m_pFormat->main.enc_type != IAmbaRecordControl::ENC_H264)
	{
		EnableWindowById(IDC_RADIO2, FALSE);
		EnableWindowById(IDC_RADIO3, FALSE);

		EnableWindowById(IDC_EDIT3, FALSE);
		EnableWindowById(IDC_EDIT4, FALSE);
		EnableWindowById(IDC_BUTTON1, FALSE);
	}
	else
	{
		EnableWindowById(IDC_RADIO2, TRUE);
		EnableWindowById(IDC_RADIO3, TRUE);
		if(G_pMuxerParam->MuxerType == 0 )
		{
			EnableWindowById(IDC_EDIT3, FALSE);
			EnableWindowById(IDC_EDIT4, FALSE);
			EnableWindowById(IDC_BUTTON1, FALSE);
		}
		else
		{
			EnableWindowById(IDC_EDIT3, TRUE);
			EnableWindowById(IDC_EDIT4, TRUE);
			EnableWindowById(IDC_BUTTON1, TRUE);
		}
	}

	if (G_pMuxerParam->MuxerType == 0)
	{
		::SendDlgItemMessage(m_hwnd, IDC_RADIO1, BM_SETCHECK, BST_CHECKED, 0);
	}
	else if (G_pMuxerParam->MuxerType == 1)
	{
		::SendDlgItemMessage(m_hwnd, IDC_RADIO2, BM_SETCHECK, BST_CHECKED, 0);
	} else if (G_pMuxerParam->MuxerType == 2)
	{
		::SendDlgItemMessage(m_hwnd, IDC_RADIO3, BM_SETCHECK, BST_CHECKED, 0);
	} else if (G_pMuxerParam->MuxerType == 3)
	{
		::SendDlgItemMessage(m_hwnd, IDC_RADIO4, BM_SETCHECK, BST_CHECKED, 0);
		EnableWindowById(IDC_EDIT3, TRUE);
		EnableWindowById(IDC_EDIT4, FALSE);
	}

	return S_OK;
}

HRESULT CAmbaMuxerParamProp::OnDeactivate()
{
	::KillTimer(m_hwnd, 1);
	return S_OK;
}

HRESULT CAmbaMuxerParamProp::OnApplyChanges()
{
	if( BST_CHECKED == ::IsDlgButtonChecked(m_hwnd,IDC_RADIO1))
	{
		G_pMuxerParam->MuxerType = 0;
	}else if ( BST_CHECKED == ::IsDlgButtonChecked(m_hwnd,IDC_RADIO2))
	{
		G_pMuxerParam->MuxerType = 1;
	}else if ( BST_CHECKED == ::IsDlgButtonChecked(m_hwnd,IDC_RADIO3))
	{
		G_pMuxerParam->MuxerType = 2;
	}else if ( BST_CHECKED == ::IsDlgButtonChecked(m_hwnd,IDC_RADIO4))
	{
		G_pMuxerParam->MuxerType = 3;
	}
	
	HWND hwnd = ::GetDlgItem(m_hwnd, IDC_EDIT3);
	if (::IsWindowEnabled(hwnd))
	{
		::GetDlgItemText(m_hwnd,IDC_EDIT3,(LPTSTR)G_pMuxerParam->MuxerFilename,256);
		hwnd = ::GetDlgItem(m_hwnd, IDC_EDIT4);
		G_pMuxerParam->MaxFileSize = EditGetInt(hwnd);
		if(G_pMuxerParam->MaxFileSize < 4)
		{
			G_pMuxerParam->MaxFileSize = 4;
		}
		else if (G_pMuxerParam->MaxFileSize > 2048)
		{
			G_pMuxerParam->MaxFileSize = 2048;
		}
	}
	return S_OK;
}

INT_PTR CAmbaMuxerParamProp::OnReceiveMessage(
	HWND hwnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		if (HIWORD(wParam) ==  BN_CLICKED)
		{
			char *filename = strrchr(G_pMuxerParam->MuxerFilename, '.');
			int file_len;
			if(filename != NULL)
			{
				file_len = filename - G_pMuxerParam->MuxerFilename;
			}
			else
			{
				file_len = strlen(G_pMuxerParam->MuxerFilename);
			}

			switch (LOWORD(wParam))
			{
			case IDC_RADIO1:
				EnableWindowById(IDC_EDIT3, FALSE);
				EnableWindowById(IDC_EDIT4, FALSE);
				EnableWindowById(IDC_BUTTON1, FALSE); 
				break;

			 case IDC_RADIO2:
				EnableWindowById(IDC_EDIT3, TRUE);
				EnableWindowById(IDC_EDIT4, TRUE);
				EnableWindowById(IDC_BUTTON1, TRUE);
				::memcpy(G_pMuxerParam->MuxerFilename + file_len,".mp4", strlen(".mp4")+1);
				::SetDlgItemText(m_hwnd,IDC_EDIT3,(LPTSTR)G_pMuxerParam->MuxerFilename);
				break;

			case IDC_RADIO3:
				EnableWindowById(IDC_EDIT3, TRUE);
				EnableWindowById(IDC_EDIT4, TRUE);
				EnableWindowById(IDC_BUTTON1, TRUE);
				::memcpy(G_pMuxerParam->MuxerFilename + file_len,".f4v", strlen(".f4v")+1);
				::SetDlgItemText(m_hwnd,IDC_EDIT3,(LPTSTR)G_pMuxerParam->MuxerFilename);
				break;

			case IDC_RADIO4:
				EnableWindowById(IDC_EDIT3, TRUE);
				EnableWindowById(IDC_EDIT4, FALSE);
				EnableWindowById(IDC_BUTTON1, TRUE); 
				if (m_pFormat->main.enc_type == IAmbaRecordControl::ENC_H264)
				{
					::memcpy(G_pMuxerParam->MuxerFilename + file_len,".264", strlen(".264")+1);
				} 
				else if (m_pFormat->main.enc_type == IAmbaRecordControl::ENC_MJPEG) 
				{
					::memcpy(G_pMuxerParam->MuxerFilename + file_len,".mjpeg", strlen(".mjpeg")+1);
				}
				::SetDlgItemText(m_hwnd,IDC_EDIT3,(LPTSTR)G_pMuxerParam->MuxerFilename);
				break;

			case IDC_BUTTON1:
				OnButtonBrowse();
				break;
			}

			SetDirty();
			return (LRESULT)1;
		}
		else if (HIWORD(wParam) == EN_CHANGE )
		{
			SetDirty();			
			return (LRESULT)1;
		}
	}

	return inherited::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
}

void CAmbaMuxerParamProp::SetDirty()
{
	m_bDirty = TRUE;
	if (m_pPageSite)
	{
		m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	}
}

void CAmbaMuxerParamProp::OnButtonBrowse()
{
	OPENFILENAME ofn;	   // common dialog box structure
	char szFile[256];	   // buffer for file name

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = m_hwnd;
	ofn.lpstrFile = szFile;
//
// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
// use the contents of szFile to initialize itself.
//
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	if( BST_CHECKED == ::IsDlgButtonChecked(m_hwnd,IDC_RADIO2))
	{
		ofn.lpstrFilter = "mp4(*.mp4)\0*.mp4\0All(*.*) \0*.*\0" ;
		ofn.lpstrDefExt = "mp4";
	}
	else if( BST_CHECKED == ::IsDlgButtonChecked(m_hwnd,IDC_RADIO3))
	{
		ofn.lpstrFilter = "f4v(*.f4v)\0*.f4v\0All(*.*) \0*.*\0" ;
		ofn.lpstrDefExt = "f4v";
	}
	else if( BST_CHECKED == ::IsDlgButtonChecked(m_hwnd,IDC_RADIO4))
	{
		if (m_pFormat->main.enc_type == IAmbaRecordControl::ENC_H264)
		{
			ofn.lpstrFilter = "H264(*.264)\0*.264\0All(*.*) \0*.*\0" ;
			ofn.lpstrDefExt = "264";
		}
		else if(m_pFormat->main.enc_type == IAmbaRecordControl::ENC_MJPEG)
		{
			ofn.lpstrFilter = "MJPEG(*.mjpeg)\0*.mjpeg\0All(*.*) \0*.*\0" ;
			ofn.lpstrDefExt = "mjpeg";
		}
	}
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_OVERWRITEPROMPT;

// Display the Open dialog box. 

	if (GetSaveFileName(&ofn)==TRUE) 
	{
		::SetDlgItemText(m_hwnd,IDC_EDIT3,ofn.lpstrFile);
	}

}
int CAmbaMuxerParamProp::EnableWindowById(WORD id, BOOL enable)
{
	HWND hwnd = ::GetDlgItem(m_hwnd, id);
	return ::EnableWindow(hwnd, enable);
}

int CAmbaMuxerParamProp::EditGetInt(HWND hwnd)
{
	LRESULT size;
	char buffer[256];

	size = ::SendMessage(hwnd, WM_GETTEXT, sizeof(buffer), (LPARAM)buffer);
	if (size > 0)
		return atoi((const char *)buffer);

	return 0;
}
