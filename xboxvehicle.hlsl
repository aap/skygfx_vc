struct VS_INPUT
{
	float4 Position		: POSITION;
	float3 Normal		: NORMAL;
	float2 TexCoord		: TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 position		: POSITION;
	float3 texcoord0	: TEXCOORD0;
	float4 color		: COLOR0;
//	float4 spec		: COLOR1;
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
float4      directDir  : register(c18);
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
	float3 V = normalize(eye - mul(world, In.Position).xyz);
	Out.texcoord0 = float3(In.TexCoord.x, In.TexCoord.y, 0.0);

//	float3 N = mul(In.Normal, worldIT).xyz;
	float3 N = mul(worldIT, In.Normal).xyz;
	Out.color = float4(0.0, 0.0, 0.0, 1.0);
	Out.color.xyz += ambient*surfProps.x;
	float4 specColor = float4(0.0, 0.0, 0.0, 0.0);

	Out.color.xyz += directDiff*surfProps.z*diffuseTerm(N, -directDir);
	specColor += directSpec*specTerm(N, -directDir, V, 25.0);
	for(int i = 0; i < 4; i++) {
		Out.color.xyz += lights[i].diff*surfProps.z*diffuseTerm(N, -lights[i].dir);
		specColor += lights[i].diff*specTerm(N, -lights[i].dir, V, 25.0);
	}
	Out.color *= matCol;
	Out.color += specColor;
//	Out.spec = specColor;

	return Out;
}
