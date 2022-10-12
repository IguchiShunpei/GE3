#include "Basic.hlsli"

Texture2D<float4> tex : register(t0);	//0番スロットに設定されたテクスチャ
SamplerState smp : register(s0);		//0番スロットに設定されたサンプラー

float4 main(VSOutput input) : SV_TARGET
{

	//return float4(tex.Sample(smp,input.uv))*color;
	//return float4(input.normal,1);//RGBをそれぞれ法線のxyz、Aを1で出力
	//return float4 (1,1,1,1);

	////鏡面反射光
	//float3 light = normalize(float3(1,-1,1));//右下奥　向きのライト
	//float brightness = dot(-light, input.normal);//光源へのベクトルと法線ベクトルの内積
	//return float4(brightness, brightness, brightness, 1);//輝度をRGBに代入して出力

	//環境光
	float3 light = normalize(float3(1,-1,1));//右下奥　向きのライト
	float diffuse = saturate(dot(-light, input.normal));//diffuseを[0,1]の範囲にclampする
	float brightness = diffuse + 0.3f;//アンビエント項を0.3として計算
	//return float4(brightness, brightness, brightness, 1);//輝度をRGBに代入して出力

	float4 texColor = float4(tex.Sample(smp, input.uv));
	return float4(texColor.rgb * brightness, texColor.a) * color;
}