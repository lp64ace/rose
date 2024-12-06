vec3 color(float t) {
	return 0.5 + 0.5 * cos(6.28318 * vec3(0.0, 0.33, 0.67) + t * 6.28318);
}

vec4 mandelbrot(vec2 uv) {
	uv = uv * 5.0f - 2.5f;

	vec2 c = vec2(-0.8, 0.156);

	vec2 z = uv;

	for (float iteration = 256.0; iteration >= 0.0; iteration--) {
		if (dot(z, z) > 4.0) {
			return vec4(color(iteration / 256.0), 1.0);
		}

		float temp = z.x * z.x - z.y * z.y + c.x;
		z.y = 2.0 * z.x * z.y + c.y;
		z.x = temp;
	}

	return vec4(color(0), 1.0f);
}

void main() {
	vec2 xy = vec2(gl_GlobalInvocationID.xy);
	vec2 uv = xy / imageSize(canvas).xy;
	imageStore(canvas, ivec2(gl_GlobalInvocationID.xy), mandelbrot(uv));
}