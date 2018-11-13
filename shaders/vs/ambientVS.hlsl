struct VS_INPUT
{
	float4 Position		: POSITION;
	float4 Prelight		: COLOR0;
	float2 TexCoord		: TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 position		: POSITION;
	float2 texcoord0	: TEXCOORD0;
	float4 color		: COLOR0;
};

float4x4    combined    : register(c0);
float4      surfProps   : register(c14);
float3      ambient     : register(c15);
float4	    matCol      : register(c16);

#define surfAmb (surfProps.x)

VS_OUTPUT
main(in VS_INPUT In)
{
	VS_OUTPUT Out;

	Out.position = mul(In.Position, combined);
	Out.texcoord0 = In.TexCoord;
	Out.color = saturate(In.Prelight + float4(ambient, 0.0f)*surfAmb)*matCol;

	return Out;
}
