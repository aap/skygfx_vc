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

float4      rampStart   : register(c36);
float4      rampEnd     : register(c37);
float3      rim         : register(c38);
float3	    viewVec     : register(c41);

VS_OUTPUT
main(in VS_INPUT In)
{
	VS_OUTPUT Out;

	Out.position = mul(In.Position, combined);
	float3 N = mul(In.Normal, (float3x3)world).xyz;
	Out.texcoord0 = In.TexCoord;

	// calculate rim light
	float f = rim.x - rim.y*dot(N, -viewVec);
	float4 r = saturate(lerp(rampEnd, rampStart, f)*rim.z);

	// calculate scene light
	float3 c = directCol*saturate(dot(N, -directDir));
	c += ambient;
	for(int i = 0; i < 4; i++)
		c += lightCol[i]*saturate(dot(N, -lightDir[i]));
	c += r.rgb;
	Out.color = float4(saturate(c), 1.0f)*matCol;

	return Out;
}
