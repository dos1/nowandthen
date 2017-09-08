#ifdef GL_ES
precision highp float;
precision highp int;
#endif

attribute vec4 al_pos;
attribute vec4 al_color;
attribute vec2 al_texcoord;
uniform mat4 al_projview_matrix;
uniform bool al_use_tex_matrix;
uniform mat4 al_tex_matrix;
varying vec4 varying_color;
varying vec2 varying_texcoord;

void main() {
	varying_color = al_color;
	if (al_use_tex_matrix) {
		vec4 uv = al_tex_matrix * vec4(al_texcoord, 0, 1);
		varying_texcoord = uv.xy;
	} else {
		varying_texcoord = al_texcoord;
	}
	gl_Position = al_projview_matrix * al_pos;
}
