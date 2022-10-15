
void main ( void ) {
	gl_Position = ProjectionMatrix * ModelViewMatrix * vec4 ( pos , 1 );
}
