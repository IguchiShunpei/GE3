#define DIRECTINPUT_VERSION    0x0800 //DirectInput�̃o�[�W�����w��
#include<windows.h>
#include<cassert>
#include<vector>
#include<string>
#include<d3dcompiler.h>
#include<DirectXMath.h>
#include<dinput.h>
#include<DirectXTex.h>
#include"Input.h"
#include"WinApp.h"
#include"DirectXCommon.h"

#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"dxguid.lib")

using namespace DirectX;
using namespace Microsoft::WRL;

// �E�B���h�E�v���V�[�W��
LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	//���b�Z�[�W�ɉ����ăQ�[���ŗL�̏������s��
	switch (msg)
	{
		//�E�B���h�E���j�����ꂽ
	case WM_DESTROY:
		//OS�ɑ΂��āA�A�v���̏I����`����
		PostQuitMessage(0);
		return 0;
	}

	//�W���̃��b�Z�[�W�������s��
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

//�萔�o�b�t�@�p�f�[�^�\����(�}�e���A��)
struct ConstBufferDataMaterial {
	XMFLOAT4 color;//�F(RGBA)
};

//�萔�o�b�t�@�p�f�[�^�\����(3D�ϊ��s��)
struct ConstBufferDataTransform {
	XMMATRIX mat; //3D�ϊ��s��
};

//3D�I�u�W�F�N�g�^
struct Object3d
{
	//�萔�o�b�t�@
	ComPtr<ID3D12Resource> constBuffTransform;

	//�萔�o�b�t�@�}�b�v(�s��p)
	ConstBufferDataTransform* constMapTransform = nullptr;

	//�A�t�B���ϊ����
	XMFLOAT3 scale = { 1,1,1 };
	XMFLOAT3 rotation = { 0,0,0 };
	XMFLOAT3 position = { 0,0,0 };

	//���[���h�ϊ��s��
	XMMATRIX matWorld;

	//�e�I�u�W�F�N�g�ւ̃|�C���^
	Object3d* parent = nullptr;
};

//�������֐�
void InitializeObject3d(Object3d* object, ID3D12Device* device)
{
	HRESULT result;

	//�q�[�v�ݒ�
	D3D12_HEAP_PROPERTIES HeapProp{};
	HeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;  //GPU�ւ̓]���p
	//���\�[�X�ݒ�
	D3D12_RESOURCE_DESC resDesc{};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Width = (sizeof(ConstBufferDataTransform) * 0xff) & ~0xff;  //256�o�C�g�A���C�������g
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//�萔�o�b�t�@�̐���
	result = device->CreateCommittedResource(
		&HeapProp,//�q�[�v�ݒ�
		D3D12_HEAP_FLAG_NONE,
		&resDesc,//���\�[�X�ݒ�
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&object->constBuffTransform));
	assert(SUCCEEDED(result));

	//�萔�o�b�t�@�̃}�b�s���O
	result = object->constBuffTransform->Map(0, nullptr, (void**)&object->constMapTransform);
	assert(SUCCEEDED(result));
}

//�X�V�֐�
void UpdateObject3d(Object3d* object, XMMATRIX& matView, XMMATRIX& matProjection)
{
	XMMATRIX matScale, matRot, matTrans;

	matScale = XMMatrixScaling(object->scale.x, object->scale.y, object->scale.z);
	matRot = XMMatrixIdentity();
	matRot *= XMMatrixRotationZ(object->rotation.z);
	matRot *= XMMatrixRotationX(object->rotation.x);
	matRot *= XMMatrixRotationY(object->rotation.y);
	matTrans = XMMatrixTranslation(object->position.x, object->position.y, object->position.z);

	object->matWorld = XMMatrixIdentity();
	object->matWorld *= matScale;
	object->matWorld *= matRot;
	object->matWorld *= matTrans;

	if (object->parent != nullptr)
	{
		object->matWorld *= object->parent->matWorld;
	}

	//�萔�o�b�t�@�փf�[�^�]��
	object->constMapTransform->mat = object->matWorld * matView * matProjection;
}

//�`��֐�
void DrawObject3d(Object3d* object, ID3D12GraphicsCommandList* commandList, D3D12_VERTEX_BUFFER_VIEW& vbView,
	D3D12_INDEX_BUFFER_VIEW& ibView, UINT numIndices)
{
	//���_�o�b�t�@�̐ݒ�
	commandList->IASetVertexBuffers(0, 1, &vbView);

	//�C���f�b�N�X�o�b�t�@�̐ݒ�
	commandList->IASetIndexBuffer(&ibView);

	//�萔�o�b�t�@�r���[(CBV)�̐ݒ�R�}���h
	commandList->SetGraphicsRootConstantBufferView(2, object->constBuffTransform->GetGPUVirtualAddress());

	//�`��R�}���h
	commandList->DrawIndexedInstanced(numIndices, 1, 0, 0, 0);
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	OutputDebugStringA("Hello,DirectX!!\n");

	//�|�C���^
	WinApp* winApp = nullptr;
	
	//WindowsAPI�̏�����
	winApp = new WinApp();
	winApp->Initialize();

	//DirectX �����������@��������

	//�|�C���^
	DirectXCommon* dxCommon = nullptr;

	//DirectX�̏�����
	dxCommon = new DirectXCommon();
	dxCommon->Initialize(winApp);

	//���͂̐���
	Input* input = nullptr;

	//���͂̏�����
	input = new Input();
	input->Initialize(winApp);

	//DirectX �����������@�����܂�

	//�`�揉��������  ��������

	//���_�f�[�^�\����
	struct Vertex
	{
		XMFLOAT3 pos; //xyz���W

		XMFLOAT3 normal; //�@���x�N�g��

		XMFLOAT2 uv;  //uv���W
	};

	//���_�f�[�^
	Vertex vertices[] = {

		//�O
		{{-5.0f,-5.0f,-5.0f},{},{0.0f,1.0f}},//����
		{{-5.0f, 5.0f,-5.0f},{},{0.0f,0.0f}},//����
		{{ 5.0f,-5.0f,-5.0f},{},{1.0f,1.0f}},//�E��
		{{ 5.0f, 5.0f,-5.0f},{},{1.0f,0.0f}},//�E��

		//��
		{{-5.0f,-5.0f, 5.0f},{},{0.0f,1.0f}},//����
		{{-5.0f, 5.0f, 5.0f},{},{0.0f,0.0f}},//����
		{{ 5.0f,-5.0f, 5.0f},{},{1.0f,1.0f}},//�E��
		{{ 5.0f, 5.0f, 5.0f},{},{1.0f,0.0f}},//�E��

		//��
		{{-5.0f,-5.0f, 5.0f},{},{0.0f,1.0f}},//����
		{{-5.0f, 5.0f, 5.0f},{},{0.0f,0.0f}},//����
		{{-5.0f,-5.0f,-5.0f},{},{1.0f,1.0f}},//�E��
		{{-5.0f, 5.0f,-5.0f},{},{1.0f,0.0f}},//�E��

		//�E
		{{ 5.0f,-5.0f, 5.0f},{},{1.0f,1.0f}},//����
		{{ 5.0f, 5.0f, 5.0f},{},{1.0f,0.0f}},//����
		{{ 5.0f,-5.0f,-5.0f},{},{0.0f,1.0f}},//�E��
		{{ 5.0f, 5.0f,-5.0f},{},{0.0f,0.0f}},//�E��

		//��
		{{-5.0f,-5.0f, 5.0f},{},{0.0f,1.0f}},//����
		{{-5.0f,-5.0f,-5.0f},{},{0.0f,0.0f}},//����
		{{ 5.0f,-5.0f, 5.0f},{},{1.0f,1.0f}},//�E��
		{{ 5.0f,-5.0f,-5.0f},{},{1.0f,0.0f}},//�E��

		//��
		{{-5.0f, 5.0f, 5.0f},{},{0.0f,1.0f}},//����
		{{-5.0f, 5.0f,-5.0f},{},{0.0f,0.0f}},//����
		{{ 5.0f, 5.0f, 5.0f},{},{1.0f,1.0f}},//�E��
		{{ 5.0f, 5.0f,-5.0f},{},{1.0f,0.0f}},//�E��
	};

	//�C���f�b�N�X�f�[�^
	unsigned short indices[] =
	{
		//�O
		 0,1,2,  //�O�p�`1��
		 2,1,3,  //�O�p�`2��

		 //���
		 6,5,4,  //�O�p�`3��
		 7,5,6,	//�O�p�`4��

		 //��
		 9,10,8,  //�O�p�`5��
		 11,10,9,  //�O�p�`6�� 

		 //�E
		 12,14,13,  //�O�p�`7��
		 13,14,15,  //�O�p�`8��

		 //��
		 16,17,18,  //�O�p�`9��
		 18,17,19,  //�O�p�`10��

		 //��
		 20,22,21,  //�O�p�`11��
		 21,22,23 ,  //�O�p�`12��
	};

	BYTE keys[256] = {};
	BYTE oldkeys[256] = {};
	//�J�����A���O��
	float angle = 0.0f;

	//���W
	XMFLOAT3 scale;
	XMFLOAT3 rotation;
	XMFLOAT3 position;
	scale = { 1.0f,1.0f,1.0f };
	rotation = { 0.0f,0.0f,0.0f };
	position = { 0.0f,0.0f,0.0f };

	HRESULT result;

	//���_�f�[�^�S�̂̃T�C�Y = ���_�f�[�^1���̃T�C�Y * ���_�̗v�f��
	UINT sizeVB = static_cast<UINT>(sizeof(vertices[0]) * _countof(vertices));

	//���_�o�b�t�@�̐ݒ�
	D3D12_HEAP_PROPERTIES heapProp{};//�q�[�v�ݒ�
	heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
	//���\�[�X�ݒ�
	D3D12_RESOURCE_DESC resDesc{};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Width = sizeVB;//���_�f�[�^�S�̂̃T�C�Y
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//���_�o�b�t�@�̍쐬
	ComPtr<ID3D12Resource>vertBuff;
	result = dxCommon->GetDevice()->CreateCommittedResource(
		&heapProp,//�q�[�v�ݒ�
		D3D12_HEAP_FLAG_NONE,
		&resDesc,//���\�[�X�ݒ�
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertBuff));
	assert(SUCCEEDED(result));

	for (int i = 0; i < _countof(indices) / 3; i++)
	{
		//�O�p�`1���Ɍv�Z����
			//�O�p�`�̃C���f�b�N�X�����o���āA�ꎞ�I�ȕϐ��ɓ����
		unsigned short indexZero = indices[i * 3 + 0];
		unsigned short indexOne = indices[i * 3 + 1];
		unsigned short indexTwo = indices[i * 3 + 2];

		//�O�p�`���\�����钸�_���W���x�N�g���ɑ��
		XMVECTOR p0 = XMLoadFloat3(&vertices[indexZero].pos);
		XMVECTOR p1 = XMLoadFloat3(&vertices[indexOne].pos);
		XMVECTOR p2 = XMLoadFloat3(&vertices[indexTwo].pos);

		//p0��p1�Ap0��p2�x�N�g�����v�Z(���Z)
		XMVECTOR v1 = XMVectorSubtract(p1, p0);
		XMVECTOR v2 = XMVectorSubtract(p2, p0);

		//�O�ς͗������琂���ȃx�N�g��
		XMVECTOR normal = XMVector3Cross(v1, v2);

		//���K��(������1�ɂ���)
		normal = XMVector3Normalize(normal);

		//���߂��@���𒸓_�f�[�^�ɑ��
		XMStoreFloat3(&vertices[indexZero].normal, normal);
		XMStoreFloat3(&vertices[indexOne].normal, normal);
		XMStoreFloat3(&vertices[indexTwo].normal, normal);
	}

	//GPU��̃o�b�t�@�ɑΉ��������z������(���C����������)���擾
	Vertex* vertMap = nullptr;
	result = vertBuff->Map(0, nullptr, (void**)&vertMap);
	assert(SUCCEEDED(result));
	//�S���_�ɑ΂���
	for (int i = 0; i < _countof(vertices); i++)
	{
		vertMap[i] = vertices[i];//���W���R�s�[
	}

	//�q���������
	vertBuff->Unmap(0, nullptr);

	//���_�o�b�t�@�[�r���[�̍쐬
	D3D12_VERTEX_BUFFER_VIEW vbView{};
	//GPU���z�A�h���X
	vbView.BufferLocation = vertBuff->GetGPUVirtualAddress();
	//���_�o�b�t�@�̃T�C�Y
	vbView.SizeInBytes = sizeVB;
	//���_1���̃f�[�^�T�C�Y
	vbView.StrideInBytes = sizeof(vertices[0]);

	ComPtr<ID3DBlob>vsBlob = nullptr;//���_�V�F�[�_�I�u�W�F�N�g
	ComPtr<ID3DBlob>psBlob = nullptr;//�s�N�Z���V�F�[�_�I�u�W�F�N�g
	ComPtr<ID3DBlob>errorBlob = nullptr;//�G���[�I�u�W�F�N�g

	//�萔�o�b�t�@�pGPU���\�[�X�|�C���^
	ComPtr<ID3D12Resource>constBuffMaterial;

	//ID3D12Resource* constBuffTransform0 = nullptr;
	////�萔�o�b�t�@�̃}�b�s���O�p�|�C���^
	//ConstBufferDataTransform* constMapTransform0 = nullptr;

	//ID3D12Resource* constBuffTransform1 = nullptr;
	////�萔�o�b�t�@�̃}�b�s���O�p�|�C���^
	//ConstBufferDataTransform* constMapTransform1 = nullptr;

	//3D�I�u�W�F�N�g�̐�
	const size_t kObjectCount = 50;
	//3D�I�u�W�F�N�g�̔z��
	Object3d object3ds[kObjectCount];

	//�q�[�v�ݒ�
	D3D12_HEAP_PROPERTIES cbHeapProp{};
	cbHeapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

	//���\�[�X�ݒ�
	D3D12_RESOURCE_DESC cbResourceDesc{};
	cbResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	cbResourceDesc.Width = (sizeof(ConstBufferDataMaterial) + 0xff) & ~0xff;
	cbResourceDesc.Height = 1;
	cbResourceDesc.DepthOrArraySize = 1;
	cbResourceDesc.MipLevels = 1;
	cbResourceDesc.SampleDesc.Count = 1;
	cbResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//3D�ϊ����\�[�X
	cbResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	cbResourceDesc.Width = (sizeof(ConstBufferDataTransform) + 0xff) & ~0xff;
	cbResourceDesc.Height = 1;
	cbResourceDesc.DepthOrArraySize = 1;
	cbResourceDesc.MipLevels = 1;
	cbResourceDesc.SampleDesc.Count = 1;
	cbResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//�萔�o�b�t�@�̐���
	result = dxCommon->GetDevice()->CreateCommittedResource(
		&cbHeapProp,//�q�[�v�ݒ�
		D3D12_HEAP_FLAG_NONE,
		&cbResourceDesc,//���\�[�X�ݒ�
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constBuffMaterial));
	assert(SUCCEEDED(result));

	for (int i = 0; i < _countof(object3ds); i++)
	{
		InitializeObject3d(&object3ds[i], dxCommon->GetDevice());

		//�������火�͐e�q�T���v��
			//�擪�ȊO�Ȃ�
		if (i > 0)
		{
			////1�O�̃I�u�W�F�N�g��e�I�u�W�F�N�g�Ƃ���
			//object3ds[i].parent = &object3ds[i - 1];

			//�e�I�u�W�F�N�g��9���̑傫��
			object3ds[i].scale = { 0.9f,0.9f,0.9f };
			//�e�I�u�W�F�N�g�ɑ΂���Z�������30�x��]
			object3ds[i].rotation = { 0.0f,0.0f,XMConvertToRadians(30.0f) };
			//�e�I�u�W�F�N�g�ɑ΂���Z������-8������
			object3ds[i].position = { 0.0f,0.0f,-8.0f };
		}
	}

	////�萔�o�b�t�@�̃}�b�s���O
	ConstBufferDataMaterial* constMapMaterial = nullptr;
	result = constBuffMaterial->Map(0, nullptr, (void**)&constMapMaterial);
	assert(SUCCEEDED(result));

	//result = constBuffTransform0->Map(0, nullptr, (void**)&constMapTransform0);
	//assert(SUCCEEDED(result));

	//result = constBuffTransform1->Map(0, nullptr, (void**)&constMapTransform1);
	//assert(SUCCEEDED(result));

	//�l���������ނƎ����I�ɓ]�������
	constMapMaterial->color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	////�P�ʍs�����
	//constMapTransform0->mat = XMMatrixIdentity();

	//�������e�s��̌v�Z	
	XMMATRIX matProjection = XMMatrixPerspectiveFovLH(
		XMConvertToRadians(45.0f),//�㉺��p45�x
		(float)WinApp::window_width / WinApp::window_height,//�A�X�y�N�g��
		0.1f, 1000.0f//�O�[�A���[
	);

	//�r���[�ϊ��s��
	XMMATRIX matView;

	XMFLOAT3 eye(0, 0, -100);//���_���W
	XMFLOAT3 target(0, 0, 0);//�����_���W
	XMFLOAT3 up(0, 1, 0);//������x�N�g��
	//�r���[�ϊ��s��̌v�Z
	matView = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));

	////���[���h�ϊ��s��
	//XMMATRIX matWorld0;
	//XMMATRIX matWorld1;
	////�X�P�[�����O�s��
	//XMMATRIX matScale0;
	//XMMATRIX matScale1;
	////��]�s��
	//XMMATRIX matRot0;
	//XMMATRIX matRot1;
	////���s�ړ��s��
	//XMMATRIX matTrans0;
	//XMMATRIX matTrans1;


	//�C���f�b�N�X�f�[�^�S�̂̃T�C�Y
	UINT sizeIB = static_cast<UINT>(sizeof(uint16_t)) * _countof(indices);

	//���\�[�X�ݒ�
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Width = sizeIB;
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//�C���f�b�N�X�o�b�t�@�̐���
	ComPtr<ID3D12Resource> indexBuff = nullptr;
	result = dxCommon->GetDevice()->CreateCommittedResource(
		&cbHeapProp,//�q�[�v�ݒ�
		D3D12_HEAP_FLAG_NONE,
		&cbResourceDesc,//���\�[�X�ݒ�
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&indexBuff));

	//�C���f�b�N�X�o�b�t�@���}�b�s���O
	uint16_t* indexMap = nullptr;
	result = indexBuff->Map(0, nullptr, (void**)&indexMap);
	//�S�C���f�b�N�X�ɑ΂���
	for (int i = 0; i < _countof(indices); i++)
	{
		indexMap[i] = indices[i];
	}
	//�}�b�s���O����
	indexBuff->Unmap(0, nullptr);

	//�C���f�b�N�X�o�b�t�@�r���[�̐���
	D3D12_INDEX_BUFFER_VIEW ibView{};
	ibView.BufferLocation = indexBuff->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = sizeIB;

	//�e�N�X�`��
	TexMetadata metaData{};
	ScratchImage scratchImg{};

	result = LoadFromWICFile(
		L"Resources/mario.jpg",
		WIC_FLAGS_NONE,
		&metaData, scratchImg
	);

	ScratchImage mipChain{};
	result = GenerateMipMaps(
		scratchImg.GetImages(), scratchImg.GetImageCount(), scratchImg.GetMetadata(),
		TEX_FILTER_DEFAULT, 0, mipChain
	);
	if (SUCCEEDED(result))
	{
		scratchImg = std::move(mipChain);
		metaData = scratchImg.GetMetadata();
	}

	//�ǂݍ��񂾃f�B�t���[�Y�e�N�X�`����SRGB�Ƃ��Ĉ���
	metaData.format = MakeSRGB(metaData.format);

	//�q�[�v�ݒ�
	D3D12_HEAP_PROPERTIES textureHeapProp{};
	textureHeapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	textureHeapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	textureHeapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;

	//���\�[�X�ݒ�
	D3D12_RESOURCE_DESC textureResourceDesc{};
	textureResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureResourceDesc.Format = metaData.format;
	textureResourceDesc.Width = metaData.width;
	textureResourceDesc.Height = (UINT)metaData.height;
	textureResourceDesc.DepthOrArraySize = (UINT16)metaData.arraySize;
	textureResourceDesc.MipLevels = (UINT16)metaData.mipLevels;
	textureResourceDesc.SampleDesc.Count = 1;

	//�e�N�X�`���o�b�t�@�𐶐�
	ComPtr<ID3D12Resource> textureBuff;
	result = dxCommon->GetDevice()->CreateCommittedResource(
		&textureHeapProp,//�q�[�v�ݒ�
		D3D12_HEAP_FLAG_NONE,
		&textureResourceDesc,//���\�[�X�ݒ�
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&textureBuff));

	for (size_t i = 0; i < metaData.mipLevels; i++)
	{
		//�~�b�v�}�b�v���x�����w�肵�ăC���[�W���擾
		const Image* img = scratchImg.GetImage(i, 0, 0);
		//�e�N�X�`���o�b�t�@�Ƀf�[�^�]��
		result = textureBuff->WriteToSubresource(
			(UINT)i,//
			nullptr,//�S�̈�փR�s�[
			img->pixels,//���f�[�^�A�h���X
			(UINT)img->rowPitch,//1���C���T�C�Y
			(UINT)img->slicePitch//�S�T�C�Y
		);
		assert(SUCCEEDED(result));
	}

	const size_t kMaxSRVCount = 2056;//SRV = �V�F�[�_���\�[�X�r���[

	//�f�X�N���v�^�q�[�v�̐ݒ�(����)
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	srvHeapDesc.NumDescriptors = kMaxSRVCount;

	//�ݒ������SRV�p�f�X�N���v�^�q�[�v�𐶐�
	ComPtr<ID3D12DescriptorHeap> srvHeap;
	result = dxCommon->GetDevice()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap));
	assert(SUCCEEDED(result));

	//SPV�q�[�v�̐擪�n���h�����擾
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = srvHeap->GetCPUDescriptorHandleForHeapStart();

	//�V�F�[�_���\�[�X�r���[�ݒ�
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = resDesc.Format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = resDesc.MipLevels;

	//�n���h���̂����ʒu�ɃV�F�[�_���\�[�X�r���[�쐬
	dxCommon->GetDevice()->CreateShaderResourceView(textureBuff.Get(), &srvDesc, srvHandle);
	//�e�N�X�`��(2����)
	TexMetadata metaData2{};
	ScratchImage scratchImg2{};

	result = LoadFromWICFile(
		L"Resources/texture.jpg",
		WIC_FLAGS_NONE,
		&metaData2, scratchImg2
	);

	result = GenerateMipMaps(
		scratchImg2.GetImages(), scratchImg2.GetImageCount(), scratchImg2.GetMetadata(),
		TEX_FILTER_DEFAULT, 0, mipChain
	);
	if (SUCCEEDED(result))
	{
		scratchImg2 = std::move(mipChain);
		metaData2 = scratchImg2.GetMetadata();
	}

	metaData2.format = MakeSRGB(metaData2.format);

	//���\�[�X�ݒ�
	D3D12_RESOURCE_DESC textureResourceDesc2{};
	textureResourceDesc2.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureResourceDesc2.Format = metaData2.format;
	textureResourceDesc2.Width = metaData2.width;
	textureResourceDesc2.Height = (UINT)metaData2.height;
	textureResourceDesc2.DepthOrArraySize = (UINT16)metaData2.arraySize;
	textureResourceDesc2.MipLevels = (UINT16)metaData2.mipLevels;
	textureResourceDesc2.SampleDesc.Count = 1;

	//�e�N�X�`���o�b�t�@�𐶐�
	ComPtr<ID3D12Resource> textureBuff2;
	result = dxCommon->GetDevice()->CreateCommittedResource(
		&textureHeapProp,//�q�[�v�ݒ�
		D3D12_HEAP_FLAG_NONE,
		&textureResourceDesc2,//���\�[�X�ݒ�
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&textureBuff2));

	for (size_t i = 0; i < metaData2.mipLevels; i++)
	{
		//�~�b�v�}�b�v���x�����w�肵�ăC���[�W���擾
		const Image* img2 = scratchImg2.GetImage(i, 0, 0);
		//�e�N�X�`���o�b�t�@�Ƀf�[�^�]��
		result = textureBuff2->WriteToSubresource(
			(UINT)i,//
			nullptr,//�S�̈�փR�s�[
			img2->pixels,//���f�[�^�A�h���X
			(UINT)img2->rowPitch,//1���C���T�C�Y
			(UINT)img2->slicePitch//�S�T�C�Y
		);
		assert(SUCCEEDED(result));
	}

	//
	UINT incrementSize = dxCommon->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	srvHandle.ptr += incrementSize;

	//�V�F�[�_���\�[�X�r���[�ݒ�
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2{};
	srvDesc2.Format = textureResourceDesc2.Format;
	srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc2.Texture2D.MipLevels = textureResourceDesc2.MipLevels;

	//�n���h���̂����ʒu�ɃV�F�[�_���\�[�X�r���[�쐬
	dxCommon->GetDevice()->CreateShaderResourceView(textureBuff2.Get(), &srvDesc2, srvHandle);

	//�f�X�N���v�^�����W�̐ݒ�
	D3D12_DESCRIPTOR_RANGE descriptorRange{};
	descriptorRange.NumDescriptors = 1;
	descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange.BaseShaderRegister = 0;
	descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//���[�g�p�����[�^
	D3D12_ROOT_PARAMETER rootParams[3] = {};
	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;//�萔�o�b�t�@�r���[
	rootParams[0].Descriptor.ShaderRegister = 0;//�萔�o�b�t�@�ԍ�
	rootParams[0].Descriptor.RegisterSpace = 0;//�f�t�H���g�l
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//�S�ẴV�F�[�_���猩����

	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;//���
	rootParams[1].DescriptorTable.pDescriptorRanges = &descriptorRange;//�f�X�N���v�^�����W
	rootParams[1].DescriptorTable.NumDescriptorRanges = 1;//�f�X�N���v�^�����W��
	rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//�S�ẴV�F�[�_���猩����

	rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;//�萔�o�b�t�@�r���[
	rootParams[2].Descriptor.ShaderRegister = 1;//�萔�o�b�t�@�ԍ�
	rootParams[2].Descriptor.RegisterSpace = 0;//�f�t�H���g�l
	rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;//�S�ẴV�F�[�_���猩����

	//�e�N�X�`���T���v���[�̐ݒ�
	D3D12_STATIC_SAMPLER_DESC samplerDesc{};
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//���J��Ԃ�
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//�c�J��Ԃ�
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;//���s�J��Ԃ�
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;//�{�[�_�[�̎��͍�
	samplerDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;//�S�ă��j�A���
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;//�~�b�v�}�b�v�ő�l
	samplerDesc.MinLOD = 0.0f;//�~�b�v�}�b�v�ŏ��l
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;//
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;//�s�N�Z���V�F�[�_����̂ݎg�p�\

	//���_�V�F�[�_�̓ǂݍ��݂ƃR���p�C��(���_�V�F�[�_�͒��_�̍��W�ϊ�)
	result = D3DCompileFromFile(
		L"Resources/shaders/BasicVS.hlsl",//�V�F�[�_�[�t�@�C����
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,//�C���N���[�h���\�ɂ���
		"main",//�G���g���[�|�C���g
		"vs_5_0",//�V�F�[�_���f���w��
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,//�f�o�b�N�p�ݒ�
		0,
		&vsBlob, &errorBlob);

	////�R���p�C���G���[�Ȃ�
	//if (FAILED(result))
	//{
	//	//errorBlob����G���[���e��stirng�^�ɃR�s�[
	//	std::string error;
	//	error.resize(errorBlob->GetBufferSize());

	//	std::copy_n((char*)errorBlob->GetBufferPointer(),
	//		errorBlob->GetBufferPointer(),
	//		error.begin());
	//	error += "\n";0000000
	//	//�G���[���e���o�̓E�B���h�E�ɕ\��
	//	OutputDebugStringA(error.c_str());
	//	assert(0);
	//}

	//�s�N�Z���V�F�[�_�̓ǂݍ��݂ƃR���p�C��(�s�N�Z���̖����͕`��F�̐ݒ�)
	result = D3DCompileFromFile(
		L"Resources/shaders/BasicPS.hlsl",//�V�F�[�_�t�@�C����
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		"main",
		"ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&psBlob, &errorBlob);

	////�R���p�C���G���[�Ȃ�
	//if (FAILED(result))
	//{
	//	//errorBlob����G���[���e��stirng�^�ɃR�s�[
	//	std::string error;
	//	error.resize(errorBlob->GetBufferSize());

	//	std::copy_n((char*)errorBlob->GetBufferPointer(),
	//		errorBlob->GetBufferPointer(),
	//		error.begin());
	//	error += "\n";
	//	//�G���[���e���o�̓E�B���h�E�ɕ\��
	//	OutputDebugStringA(error.c_str());
	//	assert(0);
	//}


	//���_���C�A�E�g
	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{
			//xyz���W
			"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0,
		},

		{
			//�@���x�N�g��
			"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0,
		},

		{
			//uv���W
			"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0,
		},
	};

	//�O���t�B�b�N�X�p�C�v���C���ݒ�
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc{};

	//�V�F�[�_�̐ݒ�
	pipelineDesc.VS.pShaderBytecode = vsBlob->GetBufferPointer();
	pipelineDesc.VS.BytecodeLength = vsBlob->GetBufferSize();
	pipelineDesc.PS.pShaderBytecode = psBlob->GetBufferPointer();
	pipelineDesc.PS.BytecodeLength = psBlob->GetBufferSize();

	//�f�v�X�X�e���V���X�e�[�g�̐ݒ�
	pipelineDesc.DepthStencilState.DepthEnable = true;//�[�x�e�X�g���s��
	pipelineDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;//�������݋���
	pipelineDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;//��������΍��i
	pipelineDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;//�[�x�l�t�H�[�}�b�g

	//�T���v���}�X�N�̐ݒ�
	pipelineDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;//�W���ݒ�

	//���X�^���C�U�̐ݒ�(���_�̃s�N�Z����)
	//pipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;//�J�����O���Ȃ�
	pipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;//�w�ʂ��J�����O
	pipelineDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;//�|���S�����h��Ԃ�
	pipelineDesc.RasterizerState.DepthClipEnable = true;//�[�x�N���b�s���O��L����

	//�u�����h�X�e�[�g
	/*pipelineDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;*///RGBA�S�Ẵ`�����l����`��
	D3D12_RENDER_TARGET_BLEND_DESC& blenddesc = pipelineDesc.BlendState.RenderTarget[0];
	blenddesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;


	//���ʐݒ�(�A���t�@�l)	
	blenddesc.BlendEnable = true; //�u�����h��L���ɂ���	
	blenddesc.BlendOpAlpha = D3D12_BLEND_OP_ADD; //���Z	
	blenddesc.SrcBlendAlpha = D3D12_BLEND_ONE; //�\�[�X�̒l��100% �g��	
	blenddesc.DestBlendAlpha = D3D12_BLEND_ZERO; //�f�X�g�̒l�� 0% �g��	
	////���Z����	
	//blenddesc.BlendOp = D3D12_BLEND_OP_ADD; //���Z	
	//blenddesc.SrcBlend = D3D12_BLEND_ONE; //�\�[�X�̒l��100% �g��	
	//blenddesc.DestBlend = D3D12_BLEND_ONE; //�f�X�g�̒l��100% �g��	
	////���Z����	
	//blenddesc.BlendOp = D3D12_BLEND_OP_REV_SUBTRACT; //�f�X�g����\�[�X�����Z	
	//blenddesc.SrcBlend = D3D12_BLEND_ONE; //�\�[�X�̒l��100% �g��	
	//blenddesc.DestBlend = D3D12_BLEND_ONE; //�f�X�g�̒l��100% �g��	
	////�F���]	
	//blenddesc.BlendOp = D3D12_BLEND_OP_ADD; //���Z	
	//blenddesc.SrcBlend = D3D12_BLEND_INV_DEST_COLOR; //1.0f-�f�X�g�J���[�̒l	
	//blenddesc.DestBlend = D3D12_BLEND_ZERO; //�g��Ȃ�	
	//����������	
	blenddesc.BlendOp = D3D12_BLEND_OP_ADD; //���Z	
	blenddesc.SrcBlend = D3D12_BLEND_SRC_ALPHA; //�\�[�X�̃A���t�@�l	
	blenddesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA; //1.0f-�\�[�X�̃A���t�@�l

	//���_���C�A�E�g�̐ݒ�
	pipelineDesc.InputLayout.pInputElementDescs = inputLayout;
	pipelineDesc.InputLayout.NumElements = _countof(inputLayout);

	//�}�`�̌`��ݒ�
	pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	//���̑��̐ݒ�
	pipelineDesc.NumRenderTargets = 1;//�`��Ώۂ�1��
	pipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;//0~255�w���RGBA
	pipelineDesc.SampleDesc.Count = 1;//1�s�N�Z���ɂ�1�񃌃��_�����O

	//���[�g�V�O�l�`��
	ComPtr<ID3D12RootSignature>rootSignature;

	//���[�g�V�O�l�`���̐ݒ�(����)
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSignatureDesc.pParameters = rootParams;//���[�g�p�����[�^�̐擪�A�h���X
	rootSignatureDesc.NumParameters = _countof(rootParams);//���[�g�p�����[�^��

	rootSignatureDesc.pStaticSamplers = &samplerDesc;
	rootSignatureDesc.NumStaticSamplers = 1;

	//���[�g�V�O�l�`���̃V���A���C�Y
	//ID3DBlob* rootSigBlob = nullptr;
	ComPtr<ID3DBlob>rootSigBlob;
	result = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSigBlob, &errorBlob);
	assert(SUCCEEDED(result));
	result = dxCommon->GetDevice()->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(result));

	//�p�C�v���C���Ƀ��[�g�V�O�l�`�����Z�b�g
	pipelineDesc.pRootSignature = rootSignature.Get();

	//�p�C�v���C���X�e�[�g�̐���
	ComPtr<ID3D12PipelineState>pipelineState;
	result = dxCommon->GetDevice()->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(&pipelineState));
	assert(SUCCEEDED(result));

	//�`�揉��������  �����܂�
		//�Q�[�����[�v
	while (true)
	{
		//Windows�̃��b�Z�[�W����
		if (winApp->ProcessMessage())
		{
			//�Q�[�����[�v�𔲂���
			break;
		}

		//////////DirectX���t���[�������@��������/////

		//���͂̍X�V
		input->Update();

		//������0�L�[��������Ă�����
		if (input->TriggerKey(DIK_0))
		{
			OutputDebugStringA("Hit 0\n"); //�o�̓E�B���h�E�́uHit 0�v�ƕ\��
		}

		//�r���[�s��̌v�Z
		//�J�����A���O���ύX
		if (input->PushKey(DIK_D) || input->PushKey(DIK_A))
		{
			if (input->PushKey(DIK_D)) { angle += XMConvertToRadians(1.0f); }
			else if (input->PushKey(DIK_A)) { angle -= XMConvertToRadians(1.0f); }

			//angle���W�A������Y������ɉ�]�B���a��-100
			eye.x = -100 * sinf(angle);
			eye.z = -100 * cosf(angle);

			matView = XMMatrixLookAtLH(
				XMLoadFloat3(&eye),     //�ǂ����猩�Ă��邩
				XMLoadFloat3(&target),	//�ǂ������Ă��邩
				XMLoadFloat3(&up));		//�J�������猩����͂ǂ�����������
		}

		if (input->PushKey(DIK_UP) || input->PushKey
		(DIK_DOWN) || input->PushKey(DIK_RIGHT) || input->PushKey(DIK_LEFT))
		{
			if (input->PushKey(DIK_UP)) { object3ds[0].position.y += 1.0f; }
			else if (input->PushKey(DIK_DOWN)) { object3ds[0].position.y -= 1.0f; }
			if (input->PushKey(DIK_RIGHT)) { object3ds[0].position.x += 1.0f; }
			else if (input->PushKey(DIK_LEFT)) { object3ds[0].position.x -= 1.0f; }
		}

		for (size_t i = 0; i < _countof(object3ds); i++)
		{
			UpdateObject3d(&object3ds[i], matView, matProjection);
		}

		//�X�y�[�X�L�[��������Ă�����
		/*if (key[DIK_SPACE])
		{
			clearColor[0] = { 0.2f };
			clearColor[1] = { 0.35f };
			clearColor[2] = { 0.6f };
		}*/

		//�`��O����
		dxCommon->PreDraw();

		//DirectX���t���[�������@�����܂�

		//4,�`��R�}���h ��������
		//�p�C�v���C���X�e�[�g�̐ݒ�R�}���h
		dxCommon->GetCommondList()->SetPipelineState(pipelineState.Get());
		//���[�g�V�O�l�`���̐ݒ�
		dxCommon->GetCommondList()->SetGraphicsRootSignature(rootSignature.Get());
		//�v���~�e�B�u�`��̐ݒ�R�}���h
		dxCommon->GetCommondList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		//���_�o�b�t�@�[�r���[�̐ݒ�R�}���h
		dxCommon->GetCommondList()->IASetVertexBuffers(0, 1, &vbView);

		//�萔�o�b�t�@�r���[(CBV)�̐ݒ�R�}���h
		dxCommon->GetCommondList()->SetGraphicsRootConstantBufferView(0, constBuffMaterial->GetGPUVirtualAddress());

		//SRV�q�[�v�ݒ�R�}���h
		dxCommon->GetCommondList()->SetDescriptorHeaps(1, srvHeap.GetAddressOf());

		//SRV�q�[�v�̐擪�n���h�����擾
		D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle = srvHeap->GetGPUDescriptorHandleForHeapStart();

		//SRV�q�[�v�̐擪�ɂ���SRV�����[�g�p�����[�^��1�Ԃɐݒ�

		if (input->PushKey(DIK_SPACE))
		{
			srvGpuHandle.ptr += incrementSize;
		}

		//SRV�q�[�v�̐擪�ɂ���SRV�����[�g�p�����[�^��1�Ԃɐݒ�
		dxCommon->GetCommondList()->SetGraphicsRootDescriptorTable(1, srvGpuHandle);

		//�C���f�b�N�X�o�b�t�@�r���[�̐ݒ�R�}���h
		dxCommon->GetCommondList()->IASetIndexBuffer(&ibView);
		for (int i = 0; i < _countof(object3ds); i++)
		{
			DrawObject3d(&object3ds[i], dxCommon->GetCommondList(), vbView, ibView, _countof(indices));
		}

		// �`��I��
		dxCommon->PostDraw();

		//4,�`��R�}���h �����܂�


		//DirectX���t���[������ �����܂�
	}

	//DirectX���
	delete dxCommon;
	//���͉��
	delete input;

	//WindowsAPI�̏I������
	winApp->Finalize();

	delete winApp;
	winApp = nullptr;

	return 0;
}
