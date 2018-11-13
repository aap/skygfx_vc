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
float3      emissive    : register(c17);
float3      prelightTweak    : register(c18);

#define surfEmiss (surfProps.w)
#define tweakMult (prelightTweak.x)
#define tweakAdd (prelightTweak.y)

VS_OUTPUT
main(in VS_INPUT In)
{
	VS_OUTPUT Out;

	Out.position = mul(In.Position, combined);
	Out.texcoord0 = In.TexCoord;
	float4 prelight = In.Prelight;
	// VC prelight works badly with VCS timecycle, so tweak a little
	prelight.xyz = saturate(prelight.xyz*tweakMult + float3(1.0,1.0,1.0)*tweakAdd);
	Out.color = saturate(prelight*float4(ambient, 1.0f) + float4(emissive, 0.0f)*surfEmiss);
	Out.color.w *= matCol.w;

	return Out;
}
