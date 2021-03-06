
struct VertexToPixel
{
	// Data type
	//  |
	//  |   Name          Semantic
	//  |    |                |
	//  v    v                v
	float4 position		: SV_POSITION;
	float3 normal		: NORMAL;
	float2 uv			: TEXCOORD;
	float3 tangent		: TANGENT;
	float4 shadowPos    : SHADOWPOS;

};

struct DirectionalLight
{
	float4 AmbientColour;
	float4 DiffuseColour;
	float3 Direction;
};

Texture2D diffuseTexture	: register(t0);
Texture2D normalTexture		: register(t1);
Texture2D shadowTexture     : register(t2);
SamplerState basicSampler	: register(s0);
SamplerComparisonState shadowSampler : register(s1);

cbuffer externalData : register(b1)
{
	DirectionalLight light;
	DirectionalLight topLight;
};

float4 LightValue(DirectionalLight newLight, float3 aNormal)
{
	float3 tempNormal = normalize(aNormal);

	float3 toLight = normalize(newLight.Direction);

	float NdotL = dot(tempNormal, -toLight);
	NdotL = saturate(NdotL);

	return newLight.AmbientColour + (newLight.DiffuseColour * NdotL);
}

float3 TangentToWorldSpace(float3 aTangent, float3 aNormal, float2 aInput)
{
	float3 normalFromMap = normalTexture.Sample(basicSampler, aInput).rgb;

	// Unpack normal from texture sample. (
	float3 unpackedNormal = normalFromMap * 2.0f - 1.0f;

	// Create TBN matrix
	float3 N = aNormal;
	float3 T = aTangent - (dot(aTangent, N) * N);
	float3 B = cross(N, T);

	float3x3 TBN = float3x3(T, B, N);

	// Transform normal from map
	float3 finalNormal = mul(unpackedNormal, TBN);

	return finalNormal;
}

float4 main(VertexToPixel input) : SV_TARGET
{
	/* Shadow Stuff Start*/
	float2 shadowUV = input.shadowPos.xy / input.shadowPos.w * 0.5f + 0.5f;
	shadowUV.y = 1.0f - shadowUV.y;

	float lightDepth = input.shadowPos.z / input.shadowPos.w;

	float shadowAmount = shadowTexture.SampleCmpLevelZero(shadowSampler, shadowUV, lightDepth);
	/* Shadow Stuff End*/

	input.normal = TangentToWorldSpace(normalize(input.tangent), normalize(input.normal), input.uv);

	float4 finalLight = (LightValue(light, input.normal) + LightValue(topLight, input.normal));

	float4 surfaceColour = diffuseTexture.Sample(basicSampler, input.normal);

	return  surfaceColour * (float4(0.35f, 0.3f, 0.3f, 1) + float4(finalLight * shadowAmount));
}

