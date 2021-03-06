struct VertexToPixel
{
	float4 position		 : SV_POSITION;
	float3 samplerDir	 : TEXCOORD;
};

TextureCube SkyTexture : register(t0);
SamplerState BasicSampler : register(s0);

float4 main(VertexToPixel input) : SV_TARGET
{
	return SkyTexture.Sample(BasicSampler, input.samplerDir);
}