void main() {
    float3 co = pos;
    
    float3 dv = float3(0.0, 0.0, 0.0);
    float contrib = 0.0;

    co = (float4x4(TargetToArmatureMatrix) * vec4(co, 1.0)).xyz;

    for (int i = 0; i < 4; i++) {
        if (weight[i] > 0.0 && defgroup[i] >= 0) {
            float4x4 mat = grp_matrices.drw_poseMatrix[defgroup[i]];
            
            dv += weight[i] * ((float4x4(mat) * float4(co, 1.0)).xyz - co);
            
            contrib += weight[i];
        }
    }

    if (contrib > 1e-3) {
        co += (1.0 / contrib) * dv;
    }

    co = (float4x4(ArmatureToTargetMatrix) * vec4(co, 1.0)).xyz;

	vData.pos = co;
	vData.frontPosition = ProjectionMatrix * ModelMatrix * float4(co, 1.0);
	vData.backPosition = ProjectionMatrix * ModelMatrix * float4(co + lightDirection * lightDistance, 1.0);
}
