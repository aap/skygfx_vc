struct VS_INPUT
{
	float4 Position		: POSITION;
	float3 Normal		: NORMAL;
	float2 TexCoord		: TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 position		: POSITION;
	float4 color		: COLOR0;
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
float4	    reflProps  : register(c34);

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
	float3 V = normalize(eye - mul(world, In.Position).xyz);
	float3 N = mul((float3x3)worldIT, In.Normal).xyz;

	Out.color = float4(0.0, 0.0, 0.0, 1.0);
	Out.color.xyz += directSpec*specTerm(N, -directDir, V, reflProps.w);
	for(int i = 0; i < 4; i++)
		Out.color.xyz += lights[i].diff.xyz*specTerm(N, -lights[i].dir, V, reflProps.w*2);
	Out.color = saturate(Out.color);

	return Out;
}
