#pragma once
#include<windows.h>
#include<wrl.h>
#include"WinApp.h"

#define DIRECTINPUT_VERSION  0x0800 //DirectInputのバージョン指定
#include <dinput.h>

class Input
{
public:
	//namespace省略
	template<class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public: //メンバ関数
	//初期化
	void Initialize(WinApp*winApp);
	//更新
	void Update();

	//長押し
	bool PushKey(BYTE keyNumber);

	//押した瞬間
	bool TriggerKey(BYTE keyNumber);

	//離した瞬間
	bool ReleasedKey(BYTE keyNumber);
private: //メンバ変数

	// DirectInputのインスタンス
	ComPtr<IDirectInput8> directInput = nullptr;

	//キーボードのデバイス
	ComPtr<IDirectInputDevice8> keyboard = nullptr;

	//全キーの状態
	BYTE key[256] = {};

	// 前回の全キーの状態
	BYTE keyPre[256] = {};

	//WindowsAPI
	WinApp* winApp = nullptr;
};

