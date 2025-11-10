void main() {
	gl_Position = ModelMatrix * vec4(pos / 128.0, 1.0);
	
	normal = transpose(mat3(ModelMatrixInverse)) * nor;
}
