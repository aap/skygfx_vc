struct VS_INPUT
{
	float4 Position         : POSITION;
	float2 TexCoord         : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 position         : POSITION;
	float2 texcoord0        : TEXCOORD0;
	float3 normal           : COLOR0;
	float3 light            : COLOR1;
};

float4x4    combined    : register(c0);
float4x4    world       : register(c4);
float3      eye         : register(c12);
float3      directDir   : register(c13);

VS_OUTPUT
main(in VS_INPUT In)
{
	VS_OUTPUT Out;

	Out.position = mul(In.Position, combined);
	Out.texcoord0 = In.TexCoord;
	float3 V = normalize(eye - mul(In.Position, world).xyz);

	Out.normal     = 0.5*(1 + float3(0, 0, 1));		 // compress
	Out.light      = 0.5*(1 + normalize(V + directDir));	 //

	return Out;
}
