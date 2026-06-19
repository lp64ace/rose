void main() {
    gl_Position = ProjectionMatrix * ModelMatrix * float4(pos, 1.0);

    normal = normalize(transpose(mat3(ModelMatrixInverse)) * nor);
}
