void main() {
#ifndef UNIFORM_ID
	id = offset + index;
#endif

	gl_Position = ProjectionMatrix * ModelMatrix * float4(pos, 1.0);
}
