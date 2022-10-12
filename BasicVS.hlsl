#include "Basic.hlsli"

VSOutput main(float4 pos : POSITION, float3 normal : NORMAL, float2 uv : TEXCOORD)
{
	VSOutput output;//ピクセルシェーダに渡す値
	//output.svpos = pos;
	output.svpos = mul(mat, pos);//座標に行輝を計算
	output.normal = normal;
	output.uv = uv;

	return output;
}