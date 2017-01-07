float4 main(uniform sampler2D Diffuse : register(s0),
            uniform sampler2D Light : register(s1),
            uniform float4 lm : register(c0),
 
            in float4 Color : COLOR0,
            in float2 Tex0 : TEXCOORD0,
            in float2 Tex1 : TEXCOORD1) : COLOR0
{
	float4 t0 = tex2D(Diffuse, Tex0);
	float4 t1 = tex2D(Light, Tex1);

	float4 col;
	col = t0*Color*(1 + lm*(2*t1-1));
	col.a = Color.a*t0.a*lm.a;

//	col = t0*Color;
	return col;
}
