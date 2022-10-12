#include<windows.h>
#include<d3d12.h>
#include<dxgi1_6.h>
#include<cassert>
#include<vector>
#include<string>
#include<d3dcompiler.h>
#include<DirectXMath.h>
#include<dinput.h>

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")

using namespace DirectX;

class Model
{
public:
	Model();
	~Model();
	void Update(ID3D12Device* device, XMFLOAT3 vertices[3], uint16_t indices[3]);
	void Draw(ID3D12GraphicsCommandList* commandList, ID3D12Resource* constBuffMaterial);

public:
	HRESULT result;

	//頂点データの全体のサイズ = 頂点データ一つ分のサイズ * 頂点データの要素数
	UINT sizeVB;

	//頂点バッファの設定
	D3D12_HEAP_PROPERTIES heapProp{};        //ヒープ設定

	//リソース設定
	D3D12_RESOURCE_DESC resDesc{};

	//頂点バッファの作成
	ID3D12Resource* vertBuff;

	//頂点バッファビューの作成
	D3D12_VERTEX_BUFFER_VIEW vbView{};

	//GPU上のバッファに対応した仮想メモリ(メインメモリ上)を取得
	XMFLOAT3* vertMap;



	//インデックスデータの全体サイズ
	UINT sizeIB;

	//インデックスバッファの生成
	ID3D12Resource* indexBuff;

	//インデックスバッファをマッピング
	uint16_t* indexMap;

	//インデックスバッファビューの作成
	D3D12_INDEX_BUFFER_VIEW ibView{};


	//頂点データ
	XMFLOAT3 vertices[3];

	//インデックスデータ
	uint16_t indices[3];
};


