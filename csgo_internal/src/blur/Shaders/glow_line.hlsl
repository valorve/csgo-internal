sampler s;

// ���������� ������ � ����� �����
//float4 lineStart;
//float4 lineEnd;
//float4 glowColor;

float4 main(float2 uv : TEXCOORD0) : SV_TARGET
{
	float4 lineStart = float4(0.f, 0.5f, 0.f, 0.f);
	float4 lineEnd = float4(1.f, 0.5f, 0.f, 0.f);
	float4 glowColor = float4(1, 1, 1, 1);

	// ��������� ������ �� ����� � �����
	float2 lineDir = lineEnd - lineStart;
	float2 pointDir = uv - lineStart;

	// ��������� ���������� �� ����� �� �����
	float t = dot(pointDir, lineDir) / dot(lineDir, lineDir);
	float2 closestPoint = lineStart + saturate(t) * lineDir;
	float distance = length(uv - closestPoint);

	// ��������� ������������� ��������� � ����������� �� ���������� �� �����
	float glowIntensity = 1.0 - saturate(distance / 0.1); // 0.1 - ������ ���������

	// ������������ ��������� ������ �����
	float4 color;

	color.rgb = float3(
		glowColor[3] * glowColor[0] * glowIntensity,
		glowColor[3] * glowColor[1] * glowIntensity,
		glowColor[3] * glowColor[2] * glowIntensity
	);

	color.a = glowColor[3] * glowIntensity;
	return color;
}