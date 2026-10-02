void main() {
    gl_Position = ProjectionMatrix * ModelMatrix * float4(pos, 1.0);

    object_id = int(uint(resource_id) & 0xFFFFu) + 1;
}
