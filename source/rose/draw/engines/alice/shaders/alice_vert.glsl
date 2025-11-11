float3 mul_m4_v3(float4x4 m, float3 v) {
    return (m * float4(v, 1.0)).xyz;
}

void main() {
    float3 co = mul_m4_v3(ArmatureMatrixInverse, pos);
    float3 no = normalize(mat3(ArmatureMatrixInverse) * nor);

    co = pos;
    
    float3 dv = float3(0.0, 0.0, 0.0);
    float3 dn = float3(0.0, 0.0, 0.0);
    float contrib = 0.0;

    for (int i = 0; i < 4; i++) {
        if (weight[i] != 0.0 && defgroup[i] >= 0) {
            float4x4 mat = grp_matrices[defgroup[i]].drw_poseMatrix;
            
            dv += weight[i] * (mul_m4_v3(mat, co) - co);
            dn += weight[i] * (mat3(mat) * no);
            
            contrib += weight[i];
        }
    }

    if (contrib > 1e-7) {
        co += (1.0 / contrib) * dv;
        no = normalize((1.0 / contrib) * dn);
    }

    co = mul_m4_v3(ArmatureMatrix, co);
    no = normalize(mat3(ArmatureMatrix) * no);
    
    gl_Position = ModelMatrix * float4(co, 1.0);

    normal = normalize(transpose(mat3(ModelMatrixInverse)) * no);
}