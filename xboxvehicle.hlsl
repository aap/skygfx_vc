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
	float3 normal		: TEXCOORD1;
	float3 pos              : TEXCOORD2;
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
	Out.pos = mul(world, In.Position).xyz;
	Out.texcoord0 = In.TexCoord;
	Out.normal = mul(worldIT, In.Normal).xyz;
	Out.color = float4(0.0, 0.0, 0.0, 1.0);
	Out.color.xyz += ambient*surfProps.x;

	return Out;
}

sampler2D tex0 : register(s0);

float4
mainPS(VS_OUTPUT In) : COLOR
{
	float3 V = normalize(eye - In.pos);
	float3 N = normalize(In.normal);

	float4 color = In.color;
	float4 specColor = float4(0.0, 0.0, 0.0, 0.0);
	color.xyz += directDiff*surfProps.z*diffuseTerm(N, -directDir);
	specColor += directSpec*specTerm(N, -directDir, V, surfProps.y);
	for(int i = 0; i < 4; i++) {
		color.xyz += lights[i].diff*surfProps.z*diffuseTerm(N, -lights[i].dir);
		specColor += lights[i].diff*specTerm(N, -lights[i].dir, V, surfProps.y*2);
	}
	color *= matCol;

	float4 result = tex2D(tex0, In.texcoord0.xy) * color;
	result += specColor;
	return result;
}
