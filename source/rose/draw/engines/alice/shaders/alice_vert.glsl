float3 mul_m4_v3(float4x4 m, float3 v) {
    return (m * float4(v, 1.0)).xyz;
}

void main() {
	float3 co = mul_m4_v3(ArmatureMatrixInverse, pos);
	float3 dv = float3(0.0, 0.0, 0.0);

	float contrib = 0.0;
	for (int i = 0; i < 4; i++) {
		if (weight[i] != 0.0 && defgroup[i] >= 0) {
			float4x4 mat = grp_matrices[defgroup[i]].drw_poseMatrix;
			dv += weight[i] * (mul_m4_v3(mat, co) - co);
			contrib += weight[i];
		}
	}

	if (contrib > 1e-3) {
		co += (1.0 / contrib) * dv;
	}

	gl_Position = ModelMatrix * float4(mul_m4_v3(ArmatureMatrix, co) / 128.0, 1.0);

	normal = transpose(mat3(ModelMatrixInverse)) * nor;
}
