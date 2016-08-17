uniform sampler2D tex : register(s0);

struct PS_INPUT
{
	float2 texcoord0        : TEXCOORD0;
	float3 normal           : COLOR0;
	float3 light            : COLOR1;
};

float4
main(PS_INPUT IN) : COLOR
{
	float4 t0 = tex2D(tex, IN.texcoord0);
	float3 n = 2*IN.normal-1;            // unpack
	float3 v = 2*IN.light-1;             //

	float s = dot(n, v);
	return s*s*s*s*s*s*s*s*t0;
}
