float3 unpack_pos(int index) {
    return float3(
        mpos[index * 3 + 0],
        mpos[index * 3 + 1],
        mpos[index * 3 + 2]
    );
}

void pack_pos(int index, float3 value) {
    pos[index * 3 + 0] = value.x;
    pos[index * 3 + 1] = value.y;
    pos[index * 3 + 2] = value.z;
}

float3 unpack_nor(int index) {
    return float3(
        float(int(mnor[index] << 22) >> 22) / 511.0f,
        float(int(mnor[index] << 12) >> 22) / 511.0f,
        float(int(mnor[index] << 2)  >> 22) / 511.0f
    );
}

void pack_nor(int index, float3 value) {
    int x = int((value.x * 511.0f)) & 0x3FF;
    int y = int((value.y * 511.0f)) & 0x3FF;
    int z = int((value.z * 511.0f)) & 0x3FF;
    nor[index] = uint(x) | uint(y) << 10 | uint(z) << 20;
}

void main() {
    int index = int(gl_GlobalInvocationID.x);
	
    float3 co = unpack_pos(index);
    float3 no = unpack_nor(index);

    float3 dv = float3(0.0, 0.0, 0.0);
    float3 dn = float3(0.0, 0.0, 0.0);
    float contrib = 0.0;

    co = (float4x4(TargetToArmatureMatrix) * vec4(co, 1.0)).xyz;

    for (int bone = 0; bone < 4; bone++) {
        if (deform[index].weight[bone] > 0.0 && deform[index].defgroup[bone] >= 0) {
            float4x4 mat = grp_matrices.drw_poseMatrix[deform[index].defgroup[bone]];
    
            dv += deform[index].weight[bone] * ((float4x4(mat) * float4(co, 1.0)).xyz - co);
            dn += deform[index].weight[bone] * ((float3x3(mat) * no).xyz);
            
            contrib += deform[index].weight[bone];
        }
    }
    
    if (contrib > 1e-3) {
        co += (1.0 / contrib) * dv;
        no = normalize(dn);
    }

    co = (float4x4(ArmatureToTargetMatrix) * vec4(co, 1.0)).xyz;

    pack_pos(index, co);
    pack_nor(index, no);
}