void main() {
    float3 co = pos;
    float3 no = nor;
    
    float3 dv = float3(0.0, 0.0, 0.0);
    float3 dn = float3(0.0, 0.0, 0.0);
    float contrib = 0.0;

    for (int i = 0; i < 4; i++) {
        if (weight[i] != 0.0 && defgroup[i] >= 0) {
            float4x4 mat = grp_matrices[defgroup[i]].drw_poseMatrix;
            
            dv += weight[i] * ((float4x4(mat) * float4(co, 1.0)).xyz - co);
            dn += weight[i] * ((float3x3(mat) * no).xyz);
            
            contrib += weight[i];
        }
    }

    if (contrib > 1e-7) {
        co += (1.0 / contrib) * dv;
        no = normalize(dn);
    }

    gl_Position = ProjectionMatrix * ModelMatrix * float4(co.xzy, 1.0);

    normal = normalize(transpose(mat3(ModelMatrixInverse)) * no.xzy);
}
