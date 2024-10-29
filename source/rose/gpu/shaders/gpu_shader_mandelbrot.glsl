vec4 mandelbrot(vec2 uv) {
	uv = uv * 5.0f - 2.5f;
	vec2 xy = vec2(0.0);
	for(float iteration = 64; iteration >= 0; iteration--) {
		if(dot(xy, xy) > 4) {
			return vec4(vec3(iteration) / 60.0, 1.0);
		}
		float temp = xy.x * xy.x - xy.y * xy.y + uv.x;
		xy.y = 2 * xy.x * xy.y + uv.y;
		xy.x = temp;
	}
	return vec4(0, 0, 0, 1.0);
}

void main() {
	vec2 xy = vec2(gl_GlobalInvocationID.xy);
	vec2 uv = xy / imageSize(canvas).xy;
	imageStore(canvas, ivec2(gl_GlobalInvocationID.xy), mandelbrot(uv));
}