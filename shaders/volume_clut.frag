#version 150

uniform sampler3D tex_mask;
uniform sampler3D tex_volume;
uniform sampler3D tex_gradients;
uniform sampler1D tex_clut;
uniform sampler2D tex_jitter;
uniform sampler1D tex_context;

uniform float visible_min;
uniform float visible_scale;
uniform vec3 volumeDimensions;
uniform bool signed_normalized;
uniform bool use_shading;
uniform float isoValue;
uniform vec3 lightDirection;
uniform vec3 minGradient;
uniform vec3 rangeGradient;
uniform float opacity_correction;    // current sampling rate divided by reference sampling rate
uniform float opacity_scale;         // scales overall opacity
uniform int render_mode;

uniform vec2 window_size;
uniform bool cursor_on;
uniform vec3 cursor_position;
uniform vec3 cursor_position_es;
uniform vec3 cursor_position_ss;
uniform float cursor_radius_ws;
uniform float cursor_radius_ss;

uniform float sampling_length;
uniform bool use_jitter;
uniform float jitter_size;
uniform vec3 camera_pos;

uniform int num_clip_planes;
uniform vec4 clip_planes[4];

#define RENDER_MODE_MIP 0
#define RENDER_MODE_VR 1
#define RENDER_MODE_ISO 2
#define AMBIENT 0.3

in vec3 fs_texcoord;
in vec3 fs_voxel_position_ws;
in vec3 fs_voxel_position_es;
in vec3 fs_voxel_position_ss;

out vec4 display_color;


float shading(vec3 samplePos)
{
	// get gradient vector and scale it back to original
	vec3 g = texture(tex_gradients, samplePos).rgb;
	g.x = g.x * rangeGradient.x + minGradient.x;
	g.y = g.y * rangeGradient.y + minGradient.y;
	g.z = g.z * rangeGradient.z + minGradient.z;

	// use normalized gradient as lighting normal
	vec3 n = normalize(g);
	return max(min(1.0, dot(n, lightDirection)), AMBIENT);
}

float cursorAlpha(float value)
{
	float x_w = (fs_voxel_position_ss.x + 1.0) * window_size.x * 0.5;
	float y_w = (fs_voxel_position_ss.y + 1.0) * window_size.y * 0.5;

	bool in_front = dot((fs_voxel_position_es - cursor_position_es), cursor_position_es) < 0.0;
	bool in_circle = (length(vec2(x_w, y_w) - cursor_position_ss.xy)) < cursor_radius_ss;

	if (in_front && in_circle) {
		float scale = texture(tex_context, value).r;
		return scale * scale;
	} else {
		return 1.0;
	}
}

void main()
{
	// should I use jittered position?
	for (int i = 0; i < num_clip_planes; i++) {
		vec4 plane = clip_planes[i];
		if (dot(fs_voxel_position_ws, plane.xyz) > plane.w) {
			discard;
		}
	}


	vec3 samplePos = fs_texcoord;

	if (use_jitter)
	{
		// stochastic jittering of sample position (need window position of frag) (MOVE WPOS TO VERTEX SHADER)
		// rayDir is NOT the world position; it should be world_pos - eye_pos_ws
		vec2 WPOS = (vec2(1.0) + fs_voxel_position_ss.xy) * window_size * 0.5;
        float jitter_amount = (texture(tex_jitter, WPOS / jitter_size).x - 0.5) * 2.0; // in [-1,1]
        vec3 jitter_direction = normalize(fs_voxel_position_ws - camera_pos);
		samplePos += jitter_amount * sampling_length * jitter_direction;
	}

	if (texture(tex_mask, samplePos).r > 0.5) {
		discard;
	}
	
    // get raw value stored in volume (normalized to [0, 1])
    float value = texture(tex_volume, samplePos).r;
    if (signed_normalized) {
        value = value * 0.5 + 0.5;
    }



	// apply value-of-interest (window) LUT
    value = (value - visible_min) * visible_scale;
    //value = max(min(1.0, value), 0.0);

	// min(max) not necessary? should be ok with sampler state

	// color/opacity from look-up table using windowed data value
	vec4 color = texture(tex_clut, value).rgba;

	if (render_mode == RENDER_MODE_MIP) {
	
	} else if (render_mode == RENDER_MODE_VR) {

		// correct opacity and associated colors based on variable sampling distance
		float alpha_stored = color.a;
		float alpha_corrected = 1.0 - pow(1.0 - alpha_stored * opacity_scale, opacity_correction);

		if (cursor_on) {
			alpha_corrected *= cursorAlpha(value);
		}

		color.a = alpha_corrected;

		color.rgb *= max(0.0, alpha_corrected / alpha_stored);

		// apply lighting
		if (use_shading) {
			color.rgb *= shading(samplePos);
		}
	} else if (render_mode == RENDER_MODE_ISO) {
		if (value < isoValue) {
			discard;
		}

		color.rgb = vec3(1.0);

		// apply lighting
		if (use_shading) {
			color.rgb *= shading(samplePos);
		}
	}

    display_color = color;
}