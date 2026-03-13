void main() {
	const vec3 light = vec3(1.0, 1.0, 1.0);

	float factor = dot(normalize(normal), normalize(light));
	
	if (forceShadowing) {
		factor = 0.5 * factor;
	}

	fragColor = vec4(1.0);
	fragColor.xyz *= clamp(factor, 0.1, 1.0);
}
