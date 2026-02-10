#pragma ROSE_REQUIRE(overlay_common_lib.glsl)

void main() {
	vec4 color1, color2;

	/**
	 * This is slow and run per vertex, but it is still faster than
	 * doing it per instance on CPU and sending it on via instance attribute.
	 */
	mat4 InstanceModelMatrixFinal = extract_matrix_packed_data(InstanceModelMatrix, color1, color2);
	vec3 normal = normalize(transpose(inverse(mat3(InstanceModelMatrixFinal))) * nor);

	/* Do lighting at an angle to avoid flat shading on front facing bone. */
	const vec3 light = vec3(0.1, 0.1, 0.8);
	float n = dot(normal, light);

	/* Smooth lighting factor. */
	const float s = 0.2; /* [0.0-0.5] range */
	float fac = clamp((n * (1.0 - s)) + s, 0.0, 1.0);
	finalColor.rgb = mix(color2.rgb, color1.rgb, fac * fac);
	finalColor.a = 0.5;

	gl_Position = ProjectionMatrix * InstanceModelMatrixFinal * vec4(pos, 1.0);
}
