void main() {
	vec3 light = vec3(1.0, 1.0, 1.0);

	fragColor = vec4(1.0);
	fragColor.xyz *= clamp(dot(normalize(normal), normalize(light)), 0.1, 1.0);
}
