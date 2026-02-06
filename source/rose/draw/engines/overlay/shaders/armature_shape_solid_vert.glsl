mat4 extract_matrix_packed_data(mat4 mat, out vec4 dataA, out vec4 dataB) {
	const float div = 1.0 / 255.0;
	int a = int(mat[0][3]);
	int b = int(mat[1][3]);
	int c = int(mat[2][3]);
	int d = int(mat[3][3]);
	dataA = vec4(a & 0xFF, a >> 8, b & 0xFF, b >> 8) * div;
	dataB = vec4(c & 0xFF, c >> 8, d & 0xFF, d >> 8) * div;
	mat[0][3] = mat[1][3] = mat[2][3] = 0.0;
	mat[3][3] = 1.0;
	return mat;
}

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
	finalColor.a = 1.0;

	gl_Position = ProjectionMatrix * InstanceModelMatrixFinal * vec4(pos, 1.0);
}
