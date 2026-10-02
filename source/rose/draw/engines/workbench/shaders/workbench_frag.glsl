void main() {
	float factor = dot(normal, -worldData.shadow_direction_ws.xyz);
	
	if (forceShadowing) {
		factor = 0.5 * factor;
	}

	fragColor = vec4(1.0);
	fragColor.xyz *= clamp(factor, 0.1, 1.0);
	objectId = uint(object_id);
}
