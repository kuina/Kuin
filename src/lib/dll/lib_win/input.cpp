#include "input.h"

#define DIRECTINPUT_VERSION (0x0800)
#include <dinput.h>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

static const int PadNum = 4;
static const int PadBtnNum = 16;
static const int PadKeyNum = 4;

static LPDIRECTINPUT8 Device = NULL;
static LPDIRECTINPUTDEVICE8 Keyboard = NULL;
static LPDIRECTINPUTDEVICE8 Pad[PadNum] = { NULL };
static U8 KeyboardState[256] = { 0 };
static DIJOYSTATE PadState[PadNum] = { 0 };
static S64 PadBtn[PadNum][PadBtnNum] = { 0 };
static U8 PadKey[PadNum][PadBtnNum][PadKeyNum] = { 0 };
static S64 Cfg[PadNum][PadBtnNum - 4] = { 0 };
static Bool EnableCfgKey = True;

static BOOL CALLBACK CBEnumJoypad(const DIDEVICEINSTANCE* lpddi, VOID* pvref);
static BOOL CALLBACK CBEnumAxis(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvref);

EXPORT_CPP void _inputInit(void* heap, S64* heap_cnt, S64 app_code, const U8* use_res_flags)
{
	InitEnvVars(heap, heap_cnt, app_code, use_res_flags);

	for (int i = 0; i < PadNum; i++)
	{
		for (int j = 0; j < PadBtnNum - 4; j++)
			Cfg[i][j] = j;
	}
	EnableCfgKey = True;
	PadKey[0][0][0] = DIK_Z;
	PadKey[0][0][1] = DIK_RETURN;
	PadKey[0][0][2] = DIK_SPACE;
	PadKey[0][1][0] = DIK_X;
	PadKey[0][1][1] = DIK_BACKSPACE;
	PadKey[0][2][0] = DIK_C;
	PadKey[0][3][0] = DIK_V;
	PadKey[0][9][0] = DIK_A;
	PadKey[0][10][0] = DIK_S;
	PadKey[0][11][0] = DIK_ESCAPE;
	PadKey[0][12][0] = DIK_LEFT;
	PadKey[0][13][0] = DIK_RIGHT;
	PadKey[0][14][0] = DIK_UP;
	PadKey[0][15][0] = DIK_DOWN;
	if (FAILED(DirectInput8Create((HINSTANCE)GetModuleHandle(NULL), 0x0800, IID_IDirectInput8, reinterpret_cast<void**>(&Device), NULL)))
		THROW(0xe9170009);
	if (FAILED(Device->CreateDevice(GUID_SysKeyboard, &Keyboard, NULL)))
		Keyboard = NULL;
	else if (FAILED(Keyboard->SetDataFormat(&c_dfDIKeyboard)))
		Keyboard = NULL;
	else if (FAILED(Keyboard->SetCooperativeLevel(NULL, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE)))
		Keyboard = NULL;
	else
		Keyboard->Acquire();
	{
		int num = 0;
		Device->EnumDevices(DI8DEVCLASS_GAMECTRL, CBEnumJoypad, &num, DIEDFL_ATTACHEDONLY);
	}
	for (int i = 0; i < PadNum; i++)
	{
		if (Pad[i] == NULL)
			continue;
		if (FAILED(Pad[i]->SetDataFormat(&c_dfDIJoystick)))
			Pad[i] = NULL;
		else if (FAILED(Pad[i]->SetCooperativeLevel(NULL, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE)))
			Pad[i] = NULL;
		else
		{
			DIPROPDWORD d;
			d.diph.dwSize = sizeof(DIPROPDWORD);
			d.diph.dwHeaderSize = sizeof(d.diph);
			d.diph.dwObj = 0;
			d.diph.dwHow = DIPH_DEVICE;
			d.dwData = DIPROPAXISMODE_ABS;
			if (FAILED(Pad[i]->SetProperty(DIPROP_AXISMODE, &d.diph)))
				Pad[i] = NULL;
			else if (FAILED(Pad[i]->EnumObjects(CBEnumAxis, &i, DIDFT_AXIS)))
				Pad[i] = NULL;
			else
			{
				HRESULT hr = Pad[i]->Poll();
				if (FAILED(hr))
				{
					int j = 0;
					while (Pad[i]->Acquire() == DIERR_INPUTLOST)
					{
						Sleep(1);
						j++;
						if (j == 1000)
						{
							Pad[i] = NULL;
							break;
						}
					}
				}
			}
		}
	}
	{
		LPDIRECTINPUTDEVICE8 tmp[PadKeyNum] = { NULL, NULL, NULL, NULL };
		int p = 0;
		for (int i = 0; i < PadKeyNum; i++)
		{
			if (Pad[i] == NULL)
				continue;
			tmp[p] = Pad[i];
			p++;
		}
		for (int i = 0; i < PadKeyNum; i++)
			Pad[i] = tmp[i];
	}
}

EXPORT_CPP void _inputFin()
{
	for (int i = 0; i < PadNum; i++)
	{
		if (Pad[i] != NULL)
		{
			Pad[i]->Unacquire();
			Pad[i]->Release();
			Pad[i] = NULL;
		}
	}
	if (Keyboard != NULL)
	{
		Keyboard->Unacquire();
		Keyboard->Release();
		Keyboard = NULL;
	}
	if (Device != NULL)
	{
		Device->Release();
		Device = NULL;
	}
}

EXPORT_CPP void _inputUpdate()
{
	if (Keyboard != NULL)
	{
		if (FAILED(Keyboard->GetDeviceState(256, &KeyboardState)))
			Keyboard->Acquire();
	}
	for (int i = 0; i < PadNum; i++)
	{
		if (Pad[i] == NULL)
		{
			ZeroMemory(&PadState[i], sizeof(DIJOYSTATE));
			continue;
		}
		if (FAILED(Pad[i]->GetDeviceState(sizeof(DIJOYSTATE), &PadState[i])))
		{
			Pad[i]->Acquire();
			ZeroMemory(&PadState[i], sizeof(DIJOYSTATE));
		}
	}
	if (EnableCfgKey)
	{
		Bool flag, flag2;
		for (int i = 0; i < PadNum; i++)
		{
			for (int j = 0; j < PadBtnNum - 4; j++)
			{
				flag = False;
				for (int k = 0; k < PadKeyNum; k++)
				{
					if (PadKey[i][j][k] != 0 && (KeyboardState[PadKey[i][j][k]] & 0x80) != 0)
					{
						flag = True;
						break;
					}
				}
				if (flag)
					PadState[i].rgbButtons[Cfg[i][j]] = 0x80;
			}
			flag = False;
			flag2 = False;
			for (int j = 0; j < PadKeyNum; j++)
			{
				if (PadKey[i][12][j] != 0 && (KeyboardState[PadKey[i][12][j]] & 0x80) != 0)
				{
					flag = True;
					break;
				}
			}
			for (int j = 0; j < PadKeyNum; j++)
			{
				if (PadKey[i][13][j] != 0 && (KeyboardState[PadKey[i][13][j]] & 0x80) != 0)
				{
					flag2 = True;
					break;
				}
			}
			if (flag && !flag2)
				PadState[i].lX = -1000;
			else if (!flag && flag2)
				PadState[i].lX = 1000;
			flag = False;
			flag2 = False;
			for (int j = 0; j < PadKeyNum; j++)
			{
				if (PadKey[i][14][j] != 0 && (KeyboardState[PadKey[i][14][j]] & 0x80) != 0)
				{
					flag = True;
					break;
				}
			}
			for (int j = 0; j < PadKeyNum; j++)
			{
				if (PadKey[i][15][j] != 0 && (KeyboardState[PadKey[i][15][j]] & 0x80) != 0)
				{
					flag2 = True;
					break;
				}
			}
			if (flag && !flag2)
				PadState[i].lY = -1000;
			else if (!flag && flag2)
				PadState[i].lY = 1000;
		}
	}
	for (int i = 0; i < PadNum; i++)
	{
		for (int j = 0; j < PadBtnNum - 4; j++)
		{
			if ((PadState[i].rgbButtons[Cfg[i][j]] & 0x80) != 0)
				PadBtn[i][j]++;
			else
				PadBtn[i][j] = 0;
		}
		if (PadState[i].lX <= -100)
			PadBtn[i][12]++;
		else
			PadBtn[i][12] = 0;
		if (PadState[i].lX >= 100)
			PadBtn[i][13]++;
		else
			PadBtn[i][13] = 0;
		if (PadState[i].lY <= -100)
			PadBtn[i][14]++;
		else
			PadBtn[i][14] = 0;
		if (PadState[i].lY >= 100)
			PadBtn[i][15]++;
		else
			PadBtn[i][15] = 0;
	}
}

EXPORT_CPP void _enableCfgKey(Bool enabled)
{
	EnableCfgKey = enabled;
}

EXPORT_CPP S64 _getCfg(S64 idx, S64 btn)
{
	THROWDBG(idx < 0 || PadNum <= idx, 0xe9170006);
	THROWDBG(btn < 0 || PadBtnNum - 4 <= btn, 0xe9170006);
	return Cfg[idx][btn];
}

EXPORT_CPP Bool _inputKey(S64 key)
{
	return (KeyboardState[key] & 0x80) != 0;
}

EXPORT_CPP void _mousePos(S64* x, S64* y)
{
	POINT point;
	GetCursorPos(&point);
	*x = static_cast<S64>(point.x);
	*y = static_cast<S64>(point.y);
}

EXPORT_CPP S64 _pad(S64 idx, S64 btn)
{
	THROWDBG(idx < 0 || PadNum <= idx, 0xe9170006);
	THROWDBG(btn < 0 || PadBtnNum <= btn, 0xe9170006);
	return PadBtn[idx][btn];
}

EXPORT_CPP void _setCfg(S64 idx, S64 btn, S64 newBtn)
{
	THROWDBG(idx < 0 || PadNum <= idx, 0xe9170006);
	THROWDBG(btn < 0 || PadBtnNum - 4 <= btn, 0xe9170006);
	THROWDBG(newBtn < 0 || PadBtnNum - 4 <= newBtn, 0xe9170006);
	for (int i = 0; i < PadBtnNum - 4; i++)
	{
		if (Cfg[idx][i] == newBtn)
		{
			// Exchange buttons.
			Cfg[idx][i] = Cfg[idx][btn];
			Cfg[idx][btn] = newBtn;
			return;
		}
	}
}

EXPORT_CPP void _setCfgKey(S64 idx, S64 btn, const U8* keys)
{
	THROWDBG(idx < 0 || PadNum <= idx, 0xe9170006);
	THROWDBG(btn < 0 || PadBtnNum <= btn, 0xe9170006);
	int n = static_cast<int>(*reinterpret_cast<const S64*>(keys + 0x08));
	THROWDBG(n < 0 || PadKeyNum < n, 0xe9170006);
	for (int i = 0; i < PadKeyNum; i++)
	{
		if (i >= n)
			PadKey[idx][btn][i] = 0;
		else
			PadKey[idx][btn][i] = static_cast<U8>((reinterpret_cast<const S64*>(keys + 0x10))[i]);
	}
}

static BOOL CALLBACK CBEnumJoypad(const DIDEVICEINSTANCE* lpddi, VOID* pvref)
{
	int* num = static_cast<int*>(pvref);
	if (FAILED(Device->CreateDevice(lpddi->guidInstance, &Pad[*num], NULL)))
	{
		Pad[*num] = NULL;
		return DIENUM_CONTINUE;
	}
	DIDEVCAPS didevcaps;
	didevcaps.dwSize = sizeof(DIDEVCAPS);
	if (FAILED(Pad[*num]->GetCapabilities(&didevcaps)))
	{
		Pad[*num]->Release();
		Pad[*num] = NULL;
		return DIENUM_CONTINUE;
	}
	(*num)++;
	return (*num) == PadNum ? DIENUM_STOP : DIENUM_CONTINUE;
}

static BOOL CALLBACK CBEnumAxis(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID pvref)
{
	int* num = static_cast<int*>(pvref);
	DIPROPRANGE d;
	d.diph.dwSize = sizeof(DIPROPRANGE);
	d.diph.dwHeaderSize = sizeof(d.diph);
	d.diph.dwObj = lpddoi->dwType;
	d.diph.dwHow = DIPH_BYID;
	d.lMin = -1000;
	d.lMax = 1000;
	if (FAILED(Pad[*num]->SetProperty(DIPROP_RANGE, &d.diph)))
		return DIENUM_STOP;
	return DIENUM_CONTINUE;
}
