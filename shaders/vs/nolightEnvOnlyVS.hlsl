struct VS_INPUT
{
	float4 Position		: POSITION;
	float3 Normal		: NORMAL;
	float2 TexCoord		: TEXCOORD0;
	float4 Color		: COLOR0;
};

struct VS_OUTPUT
{
	float4 position		: POSITION;
	float2 texcoord0	: TEXCOORD0;
	float4 color		: COLOR0;
};

float4x4    combined    : register(c0);
float4x4    world       : register(c4);
float4x4    tex         : register(c8);

VS_OUTPUT
main(in VS_INPUT In)
{
	VS_OUTPUT Out;

	Out.position = mul(In.Position, combined);
	float3 N = normalize(mul(In.Normal, (float3x3)world).xyz);	// NORMAL MAT
	Out.texcoord0 = mul(float4(N, 1.0), tex).xy;
	Out.color = In.Color;

	return Out;
}
