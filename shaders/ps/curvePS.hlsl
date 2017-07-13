uniform sampler2D tex : register(s0);
uniform sampler2D map : register(s1);
float v : register(c0);

struct PS_INPUT
{
	float3 texcoord0	: TEXCOORD0;
};

float
scale(float low, float high, float x)
{
	return saturate(x/(high-low) - low/(high-low));
}

float4
main(PS_INPUT IN) : COLOR
{
	float4 c = tex2D(tex, IN.texcoord0.xy);
	c.a = 1.0;
	c.r = tex2D(map, float2(c.r, v)).r;
	c.g = tex2D(map, float2(c.g, v)).g;
	c.b = tex2D(map, float2(c.b, v)).b;

//	float low = 16/255.0;
//	float high = 235/255.0;
//
//	c.r = scale(low, high, c.r);
//	c.g = scale(low, high, c.g);
//	c.b = scale(low, high, c.b);
	
//	float3 low = float3(16, 16, 16)/255.0;
//	float3 high = float3(240, 240, 240)/255.0;
//	c.r = lerp(low, high, c.r).r;
//	c.g = lerp(low, high, c.g).g;
//	c.b = lerp(low, high, c.b).b;

	//c = pow(abs(c), 1/2.2);
	return c;
}
