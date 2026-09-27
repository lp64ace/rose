void main() {
	vec3 offset = vec3(worldData.viewport_size_inv, 0.0);
	vec2 uv = uvcoordsvar.st;

	uint center_id = texture(objectIdBuffer, uv).r;
	uvec4 adjacent_ids = uvec4(texture(objectIdBuffer, uv + offset.zy * 0.5).r,
							   texture(objectIdBuffer, uv - offset.zy * 0.5).r,
							   texture(objectIdBuffer, uv + offset.xz * 0.5).r,
							   texture(objectIdBuffer, uv - offset.xz * 0.5).r);

	float outline_opacity = floor(1.75 - dot(vec4(equal(uvec4(center_id), adjacent_ids)), vec4(0.25)));
	fragColor = worldData.object_outline_color * outline_opacity;
}
