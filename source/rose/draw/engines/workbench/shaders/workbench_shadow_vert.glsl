void main() {
	vData.pos = pos;
	vData.frontPosition = ProjectionMatrix * ModelMatrix * float4(pos + lightDirection * 1e-3, 1.0);
	vData.backPosition = ProjectionMatrix * ModelMatrix * float4(pos + lightDirection * lightDistance, 1.0);
}
