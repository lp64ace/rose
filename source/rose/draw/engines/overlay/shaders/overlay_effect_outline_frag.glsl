void main() {
	vec3 offset = vec3(worldData.viewport_size_inv, 0.0);
	vec2 uv = uvcoordsvar.st;

	uint center_id = texture(objectIdBuffer, uv).r;
	uvec4 adjacent_ids = uvec4(texture(objectIdBuffer, uv + offset.zy * 1.0).r,
							   texture(objectIdBuffer, uv - offset.zy * 1.0).r,
							   texture(objectIdBuffer, uv + offset.xz * 1.0).r,
							   texture(objectIdBuffer, uv - offset.xz * 1.0).r);

	float outline_opacity = 2.0 - dot(vec4(equal(uvec4(center_id), adjacent_ids)), vec4(0.5));
	fragColor = worldData.object_outline_color * outline_opacity;
}
