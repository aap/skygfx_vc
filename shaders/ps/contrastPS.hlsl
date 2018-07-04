struct PS_INPUT
{
	float4 position		: POSITION;
	float3 texcoord0	: TEXCOORD0;
	float4 color		: COLOR0;
};

uniform float3 contrastMult : register(c3);
uniform float3 contrastAdd : register(c4);

sampler2D tex0 : register(s0);

float4
main(PS_INPUT In) : COLOR
{
	float4 dst = tex2D(tex0, In.texcoord0.xy);

	dst.rgb = dst.rgb*contrastMult + contrastAdd;
	dst.a = 1.0;
	return dst;
}
