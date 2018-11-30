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
float3      eye         : register(c12);
float3      directDir   : register(c13);
float3      ambient     : register(c15);
float4	    matCol      : register(c16);
float3      directCol   : register(c17);
float3      lightDir[4] : register(c18);
float3      lightCol[4] : register(c22);

float4	    directSpec  : register(c26);

VS_OUTPUT
main(in VS_INPUT In)
{
	VS_OUTPUT Out;

	Out.position = mul(In.Position, combined);
	Out.texcoord0 = In.TexCoord;
	float3 N = normalize(mul(In.Normal, (float3x3)world).xyz);
	float3 V = normalize(eye - mul(In.Position, world).xyz);

	float3 c = directCol*saturate(dot(N, -directDir));
	c += ambient;
	for(int i = 0; i < 4; i++)
		c += lightCol[i]*saturate(dot(N, -lightDir[i]));
	Out.color = float4(saturate(c), 1.0f)*matCol;

	N = mul(N, (float3x3)tex);	// to view space
	N = -N;
	Out.texcoord1.xy = N.xy*0.5 + 0.5;

	return Out;
}