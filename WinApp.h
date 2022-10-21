#pragma once
#include <windows.h>

class WinApp
{
public://静的メンバ関数
	static LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	//ウィンドウサイズ
	static const int window_width = 1280; //横
	static const int window_height = 720; //縦

public://メンバ関数
	//初期化
	void Initialize();

	//終了
	void Finalize();

	//メッセージの処理
	bool ProcessMessage();

	//getter
	HWND GetHwnd() const { return hwnd_; }
	HINSTANCE GetHInstance() const { return w.hInstance; }
private:
	//ウィンドウハンドル
	HWND hwnd_ = nullptr;

	//ウィンドウクラスの設定
	WNDCLASSEX w{};
};

