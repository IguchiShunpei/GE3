#include"DrawingObject.h"

Model::Model() {};

Model::~Model() {};

void Model::Update(ID3D12Device* device, XMFLOAT3 vertices[3], uint16_t indices[3])
{
	for (int i = 0; i < 3; i++)
	{
		this->vertices[i] = vertices[i];
		this->indices[i] = indices[i];
	}

	//頂点データの全体のサイズ = 頂点データ一つ分のサイズ * 頂点データの要素数
	sizeVB = static_cast<UINT>(sizeof(XMFLOAT3) * _countof(this->vertices));

	//頂点バッファの設定
       
	heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;  //GPUへの転送用

	
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Width = sizeVB; //頂点データ全体のサイズ
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//頂点バッファの作成
	vertBuff = nullptr;
	result = device->CreateCommittedResource
	(
		&heapProp,  //ヒープ設定
		D3D12_HEAP_FLAG_NONE,
		&resDesc,   //リソース設定
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuff)
	);
	assert(SUCCEEDED(result));

	//GPU上のバッファに対応した仮想メモリ(メインメモリ上)を取得
	vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	assert(SUCCEEDED(result));

	//前頂点に対して
	for (int i = 0; i < _countof(this->vertices); i++)
	{
		vertMap[i] = vertices[i];   //座標をコピー
	}

	//繋がりを解除
	vertBuff->Unmap(0, nullptr);

	//GPU仮想アドレス
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();
	//頂点バッファのサイズ
	vbView.SizeInBytes = sizeVB;
	//頂点一つ分のデータサイズ
	vbView.StrideInBytes = sizeof(XMFLOAT3);

	//インデックスデータの全体サイズ
	sizeIB = static_cast<UINT>(sizeof(uint16_t) * _countof(this->indices));

	//リソース設定
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Width = sizeIB; //頂点データ全体のサイズ
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//インデックスバッファの生成
	indexBuff = nullptr;
	result = device->CreateCommittedResource
	(
		&heapProp,//ヒープ設定
		D3D12_HEAP_FLAG_NONE,
		&resDesc,//リソース設定
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&indexBuff)
	);

	//インデックスバッファをマッピング
	indexMap = nullptr;
	result = indexBuff->Map(0, nullptr, (void**)&indexMap);
	//全インデックスに対して
	for (int i = 0; i < _countof(this->indices); i++)
	{
		indexMap[i] = indices[i]; //インデックスをコピー
	}
	//マッピング解除
	indexBuff->Unmap(0, nullptr);

	ibView.BufferLocation = indexBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = sizeIB;

};

void Model::Draw(ID3D12GraphicsCommandList* commandList, ID3D12Resource* constBuffMaterial)
{
	//頂点バッファビューの設定コマンド
	commandList->IASetVertexBuffers(0, 1, &vbView);
	//インデックスバッファビューの設定コマンド
	commandList->IASetIndexBuffer(&ibView);
	//定数バッファビュー(CBV)の設定コマンド
	commandList->SetGraphicsRootConstantBufferView(0, constBuffMaterial->GetGPUVirtualAddress());
	//描画コマンド
	commandList->DrawIndexedInstanced(_countof(this->indices), 1, 0, 0, 0);  //全ての頂点を使って描画
};