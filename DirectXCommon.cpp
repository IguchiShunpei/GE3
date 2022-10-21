#include "DirectXCommon.h"

#include <cassert>
#include <vector>
#include <thread>
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")

using namespace Microsoft::WRL;

void DirectXCommon::Initialize(WinApp*winApp)
{
	//NULL検出
	assert(winApp);

	//メンバ変数に記録
	this->winApp = winApp;

	// FPS固定初期化
	InitializeFixFPS();

	//デバイスの初期化
	InitializeDevice();

	//コマンド関連の初期化
	InitializeCommand();

	//スワップチェーンの初期化
	InitializeSwapChain();

	//レンダーターゲットビューの初期化
	InitializeRendeTargetView();

	//深度バッファビューの初期化
	InitializeDepthBuffer();

	//フェンスの初期化
	InitializeFence();


}

void DirectXCommon::InitializeDevice()
{
	#ifdef _DEBUG
	//デバッグレイヤーをオンに
	ComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		debugController->EnableDebugLayer();
	}
#endif

	//DXGIファクトリーの生成
	result = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory_));
	assert(SUCCEEDED(result));

	//アダプターの列挙用
	std::vector<ComPtr<IDXGIAdapter4>> adapters;
	//ここに特定の名前を持つアダプターオブジェクトが入る
	ComPtr<IDXGIAdapter4> tmpAdapter;

	//パフォーマンスが高いものから順に、アダプターに列挙する
	for (UINT i = 0;
		dxgiFactory_->EnumAdapterByGpuPreference(i,
			DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
			IID_PPV_ARGS(&tmpAdapter)) != DXGI_ERROR_NOT_FOUND;
		i++) {
		//動的配列に追加する
		adapters.push_back(tmpAdapter);
	}

	//妥当なアダプタを選別する
	for (size_t i = 0; i < adapters.size(); i++)
	{
		DXGI_ADAPTER_DESC3 adapterDesc;
		//アダプターの情報を取得する
		adapters[i]->GetDesc3(&adapterDesc);

		//ソフトウェアデバイスを回避
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE))
		{
			//デバイスを採用してループを抜ける
			tmpAdapter = adapters[i].Get();
			break;
		}
	}

	//対応レベルの配列
	D3D_FEATURE_LEVEL levels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	D3D_FEATURE_LEVEL featureLevel;

	for (size_t i = 0; i < _countof(levels); i++)
	{
		//採用したアダプターでデバイスを生成
		result = D3D12CreateDevice(tmpAdapter.Get(), levels[i],
			IID_PPV_ARGS(&device_));
		if (result == S_OK)
		{
			//デバイスを生成できた時点でループを抜ける
			featureLevel = levels[i];
			break;
		}
	}
}

void DirectXCommon::InitializeCommand()
{
	//コマンドアロケータを生成
	result = device_->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&cmdAllocator_));
	assert(SUCCEEDED(result));

	//コマンドリストを生成
	result = device_->CreateCommandList(0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		cmdAllocator_.Get(), nullptr,
		IID_PPV_ARGS(&commandList_));
	assert(SUCCEEDED(result));

	//コマンドキューの設定
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	//コマンドキューの生成
	result = device_->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue_));
	assert(SUCCEEDED(result));
}

void DirectXCommon::InitializeSwapChain()
{
	//スワップチェーンの設定
	swapChainDesc_.Width = 1280;
	swapChainDesc_.Height = 720;
	swapChainDesc_.Format = DXGI_FORMAT_R8G8B8A8_UNORM;         //色情報の書式
	swapChainDesc_.SampleDesc.Count = 1;                        //マルチサンプルしない
	swapChainDesc_.BufferUsage = DXGI_USAGE_BACK_BUFFER; //バックバッファ用
	swapChainDesc_.BufferCount = 2;                             //バッファ数を2津に設定
	swapChainDesc_.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;  //フリップ後は破棄
	swapChainDesc_.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	//スワップチェーンの生成
	ComPtr<IDXGISwapChain1> swapChain1;
	result = dxgiFactory_->CreateSwapChainForHwnd(
		commandQueue_.Get(), winApp->GetHwnd(), &swapChainDesc_, nullptr, nullptr,
		&swapChain1);

	//生成したIDXGISwapChain1のオブジェクトをIDXGISwapChain4に変換する
	swapChain1.As(&swapChain_);
	assert(SUCCEEDED(result));
}

void DirectXCommon::InitializeRendeTargetView()
{
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;       //レンダーターゲットビュー
	rtvHeapDesc.NumDescriptors = swapChainDesc_.BufferCount;  //裏表の2つ

	//デスクリプタヒープの生成
	device_->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap_));

	//バックバッファ
	backBuffers_.resize(swapChainDesc_.BufferCount);

	//スワップチェーンのすべてのバッファについて処理する
	for (size_t i = 0; i < backBuffers_.size(); i++)
	{
		//スワップチェーンからバッファを取得
		swapChain_->GetBuffer((UINT)i, IID_PPV_ARGS(&backBuffers_[i]));
		//デスクリプタヒープのハンドルを取得
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
		//裏か表かでアドレスがずれる
		rtvHandle.ptr += i * device_->GetDescriptorHandleIncrementSize(rtvHeapDesc.Type);
		//レンダーターゲットビューの設定
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		//シェーダーの計算結果をSRGBに変換して書き込む
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		//レンダーターゲットビューの生成
		device_->CreateRenderTargetView(backBuffers_[i].Get(), &rtvDesc, rtvHandle);
	}
}

void DirectXCommon::InitializeDepthBuffer()
{
	//深度バッファのリソース設定
	//リソース設定
	D3D12_RESOURCE_DESC depthResourceDesc{};
	depthResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthResourceDesc.Width = WinApp::window_width;//レンダーターゲットに合わせる
	depthResourceDesc.Height = WinApp::window_height;//レンダーターゲットに合わせる
	depthResourceDesc.DepthOrArraySize = 1;
	depthResourceDesc.Format = DXGI_FORMAT_D32_FLOAT;//深度値フォーマット
	depthResourceDesc.SampleDesc.Count = 1;
	depthResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;//デプスステンシル

	//深度値用ヒーププロパティ
	D3D12_HEAP_PROPERTIES depthHeapProp{};
	depthHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	//深度値のクリア設定
	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f;//深度値1.0f(最大値)でクリア
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;//深度値フォーマット

	//深度バッファの生成
	result = device_->CreateCommittedResource(
		&depthHeapProp,
		D3D12_HEAP_FLAG_NONE,
		&depthResourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,//深度値書き込みに使用
		&depthClearValue,
		IID_PPV_ARGS(&depthBuff_));

	//深度ビュー用デスクリプタヒープの設定
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	result = device_->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap_));

	//深度ステンシルビューの生成
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	device_->CreateDepthStencilView(
		depthBuff_.Get(),
		&dsvDesc,
		dsvHeap_->GetCPUDescriptorHandleForHeapStart());
}

void DirectXCommon::InitializeFence()
{
	//フェンスの生成
	result = device_->CreateFence(fenceVal_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
}

void DirectXCommon::PreDraw()
{
	//バックバッファの番号を取得(2つなので0番か1番)
	UINT bbIndex = swapChain_->GetCurrentBackBufferIndex();

	//リソースバリアでバックバッファの状態を変更(表示用→描画対象)
	barrierDesc_.Transition.pResource = backBuffers_[bbIndex].Get();          //バックバッファを指定
	barrierDesc_.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;       //表示状態から
	barrierDesc_.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;  //描画状態へ
	commandList_->ResourceBarrier(1, &barrierDesc_);

	//2,描画先の変更
	//レンダーターゲットビューのハンドルを取得
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += bbIndex * device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	//深度ステンシルビュー用デスクリプタヒープのハンドルを取得
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap_->GetCPUDescriptorHandleForHeapStart();
	
    //レンダーターゲットをセット 
	commandList_->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);

	//3,画面クリア
	FLOAT clearColor[] = { 0.1f,0.25f,0.5f,0.0f }; //青っぽい色

	//レンダーターゲットクリア
	commandList_->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	//深度バッファクリア
	commandList_->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	//ビューポート設定コマンド
	D3D12_VIEWPORT viewport{};
	viewport.Width = WinApp::window_width;
	viewport.Height = WinApp::window_height;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	//ビューポート設定コマンドを、コマンドリストに積む
	commandList_->RSSetViewports(1, &viewport);

	//シザー矩形
	D3D12_RECT scissorRect{};
	scissorRect.left = 0;
	scissorRect.right = WinApp::window_width;
	scissorRect.top = 0;
	scissorRect.bottom = WinApp::window_height;
	//シザー矩形設定コマンドを、コマンドリストに積む
	commandList_->RSSetScissorRects(1, &scissorRect);
}

void DirectXCommon::PostDraw()
{
	//バックバッファの番号を取得(2つなので0番か1番)
	UINT bbIndex = swapChain_->GetCurrentBackBufferIndex();

	//リソースバリアを戻す
	barrierDesc_.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET; //描画状態から
	barrierDesc_.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;        //表示状態へ
	commandList_->ResourceBarrier(1, &barrierDesc_);

	// 命令のクローズ
	result = commandList_->Close();
	assert(SUCCEEDED(result));
	// コマンドリストの実行
	ID3D12CommandList* commandLists[] = { commandList_.Get() };
	commandQueue_->ExecuteCommandLists(1, commandLists);

	// 画面に表示するバッファをフリップ（裏表の入替え）
	result = swapChain_->Present(1, 0);
	assert(SUCCEEDED(result));

	// コマンドの実行完了を待つ
	commandQueue_->Signal(fence_.Get(), ++fenceVal_);
	if (fence_->GetCompletedValue() != fenceVal_) {
		HANDLE event = CreateEvent(nullptr, false, false, nullptr);
		fence_->SetEventOnCompletion(fenceVal_, event);
		WaitForSingleObject(event, INFINITE);
		CloseHandle(event);
	}

	// FPS固定
	UpdateFixFPS();

	// キューをクリア
	result = cmdAllocator_->Reset();
	assert(SUCCEEDED(result));
	// 再びコマンドリストを貯める準備
	result = commandList_->Reset(cmdAllocator_.Get(), nullptr);
	assert(SUCCEEDED(result));
}

void DirectXCommon::InitializeFixFPS()
{
	// 現在時間を記録する
	reference_ = std::chrono::steady_clock::now();
}

void DirectXCommon::UpdateFixFPS()
{
	// 1/60秒ぴったりの時間
	const std::chrono::microseconds kMinTime(uint64_t(1000000.0f / 60.0f));

	// 1/60秒よりわずかに短い時間
	const std::chrono::microseconds kMinCheckTime(uint64_t(1000000.0f / 65.0f));

	// 現在時間を取得する
	std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

	// 前回記録からの経過時間を取得する
	std::chrono::microseconds elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - reference_);

	// 1/60秒（よりわずかに短い時間）経っていない場合
	if (elapsed < kMinCheckTime) {
		// 1/60秒経過するまで微小なスリープを繰り返す
		while (std::chrono::steady_clock::now() - reference_ < kMinTime) {
			// 1マイクロ秒スリープ
			std::this_thread::sleep_for(std::chrono::microseconds(1));
		}
	}

	// 現在時間を記録する
	reference_ = std::chrono::steady_clock::now();
}