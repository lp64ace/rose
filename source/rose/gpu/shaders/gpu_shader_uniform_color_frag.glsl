#pragma ROSE_REQUIRE(gpu_shader_colorspace_lib.glsl)

void main() {
	fragColor = rose_srgb_to_framebuffer_space(color);
}
