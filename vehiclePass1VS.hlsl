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
	float4 reflcolor	: COLOR1;
};

struct Directional {
	float3 dir;
	float4 diff;
};

float4x4    world      : register(c0);
float4x4    worldIT    : register(c4);
float4x4    view       : register(c8);
float4x4    proj       : register(c12);
float3      eye        : register(c16);
float4      ambient    : register(c17);
float3      directDir  : register(c18);
float4      directDiff : register(c19);
float4      directSpec : register(c20);
Directional lights[4]  : register(c21);
// per mesh
float4	    matCol     : register(c29);
float4	    surfProps  : register(c30);

float
specTerm(float3 N, float3 L, float3 V, float power)
{
	return pow(saturate(dot(N, normalize(V + L))), power);
}

float
diffuseTerm(float3 N, float3 L)
{
	return saturate(dot(N, L));
}

VS_OUTPUT
mainVS(in VS_INPUT In)
{
	VS_OUTPUT Out;

	Out.position = mul(proj, mul(view, mul(world, In.Position)));
	Out.texcoord0 = In.TexCoord;
	float3 N = mul((float3x3)worldIT, In.Normal).xyz;

	Out.color = float4(0.0, 0.0, 0.0, 1.0);
	Out.color.xyz += ambient.xyz;
	Out.color.xyz += diffuseTerm(N, -directDir);
	for(int i = 0; i < 4; i++)
		Out.color.xyz += lights[i].diff.xyz*diffuseTerm(N, -lights[i].dir);
	Out.color *= matCol;
	Out.color = saturate(Out.color);

	float3 V = normalize(eye - mul(world, In.Position).xyz);
	float a = dot(V, N)*2.0;
	float3 U = N*a - V;
//	U = mul(view, U);
	Out.texcoord1.xy = U.xy*0.5 + 0.5;
	float b = 1.0 - saturate(dot(V, N));
	a = b*b;
	a = a*a;
	b = b*a;
	b = b*(1.0-surfProps.z) + surfProps.z;
	Out.reflcolor = b*surfProps.x;
//	Out.reflcolor *= float4(141, 144, 141, 255)/255.0;

	return Out;
}
