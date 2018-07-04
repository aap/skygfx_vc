struct VS_INPUT
{
	float4 Position		: POSITION;
	float3 Normal		: NORMAL;
	float2 TexCoord		: TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 position		: POSITION;
	float2 texcoord0	: TEXCOORD0;
	float2 texcoord1	: TEXCOORD1;
	float4 color		: COLOR0;
};

float4x4    combined    : register(c0);
float4x4    world       : register(c4);
float4x4    tex         : register(c8);
float3      directDir   : register(c13);
float4      surfProps   : register(c14);
float3      ambient     : register(c15);
float4	    matCol      : register(c16);
float3      directCol   : register(c17);
float3      lightDir[4] : register(c18);
float3      lightCol[4] : register(c22);

#define surfAmb (surfProps.x)
#define surfDiff (surfProps.z)

VS_OUTPUT
main(in VS_INPUT In)
{
	VS_OUTPUT Out;

	Out.position = mul(In.Position, combined);
	Out.texcoord0 = In.TexCoord;
	float3 N = normalize(mul(In.Normal, (float3x3)world).xyz);	// NORMAL MAT
	Out.texcoord1 = mul(float4(N, 1.0), tex).xy;

	float3 c = ambient*surfAmb*matCol.xyz;
	c += directCol*saturate(dot(N, -directDir))*surfDiff*matCol.xyz;
	for(int i = 0; i < 4; i++)
		c += lightCol[i]*saturate(dot(N, -lightDir[i]))*surfDiff*matCol.xyz;
	Out.color = float4(saturate(c), matCol.w);

	return Out;
}
