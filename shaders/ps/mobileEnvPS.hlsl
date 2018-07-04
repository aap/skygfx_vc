float4 main(uniform sampler2D Diffuse : register(s0),
            uniform sampler2D EnvMap : register(s1),
            uniform float4 EnvCoeff : register(c0),
 
            in float4 Color : COLOR0,
            in float2 Tex0 : TEXCOORD0,
            in float2 Tex1 : TEXCOORD1) : COLOR0
{
	float4 t0 = tex2D(Diffuse, Tex0);
	float4 t1 = tex2D(EnvMap, Tex1);

	return t0*(Color + float4(t1.xyz, 0.0)*EnvCoeff);
}
