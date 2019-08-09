uniform sampler2D texture;
uniform vec3 targetColor;
uniform float amount;

void main() {
	// lookup the pixel in the texture
	vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);

	if (pixel.a != 0.0) {
		vec3 originalColor = vec3(pixel.r, pixel.g, pixel.b);
		gl_FragColor = vec4(mix(originalColor, targetColor, amount), pixel.a);
	} else {
		gl_FragColor = pixel;
	}
}