#pragma once

#include <obs-module.h>

#ifdef __cplusplus
extern "C" {
#endif

struct tilegen_settings {
	// Grid
	int grid_pattern;
	float density;
	bool square_cells;
	float gap;
	float seed;
	float pattern_scale;
	float pattern_offset_x;
	float pattern_offset_y;

	// Shape
	int shape_type;
	float shape_size;
	float edge_softness;
	float rotation_deg;
	float auto_rotate_speed;
	int shape_sides;
	float star_sharpness;
	float stroke_width;
	struct vec4 stroke_color;

	// Shape geometry (new)
	float line_thickness;
	float rounded_square_radius;

	// Mix
	int shape_type_b;
	float shape_mix_chance;
	float shape_mix_blend_width;
	bool shape_mix_independent_motion;

	// Image / source
	int use_source_instead_of_image;
	char *image_path;
	char *source_name;
	float image_aspect_override;
	int image_or_source_loaded;

	// Atlas
	char *font_atlas_path;
	int font_cols;
	int font_rows;
	int font_index;
	float font_aspect_override;

	// Color
	struct vec4 base_color;
	struct vec4 secondary_color;
	int color_mode;
	float color_mix;

	// Palette (new: up to 4 colors)
	int palette_count;
	struct vec4 color_b;
	struct vec4 color_c;
	struct vec4 color_d;

	// Variation
	float visibility_chance;
	float size_variation;
	float rotation_variation;
	float hue_variation;
	float color_jitter;
	float twinkle_amount;
	float twinkle_speed;

	// Movement
	float scroll_speed;
	float scroll_angle;
	float drift_amount;
	float drift_speed;
	float pulse_amount;
	float pulse_speed;

	// Advanced motion (new)
	float wave_amount;
	float wave_frequency;
	float wave_speed;
	float orbit_amount;
	float orbit_speed;

	// Gradient (new)
	int gradient_axis;
	float gradient_size_amount;
	float gradient_hue_amount;

	// Vignette
	float vignette_amount;
	float vignette_size;
	float vignette_softness;

	// Glow
	float glow_amount;
	float glow_radius;
	struct vec4 glow_color;

	// Image transform
	float image_rotation;
	float image_scale;
	float image_offset_x;
	float image_offset_y;

	// Image fit & mask (new)
	int image_fit_mode;
	bool image_use_shape_mask;
	int image_mask_shape;

	// Performance
	float render_scale;

	// Predefined preset selection (transient)
	int predefined_preset;
	// Background
	int use_internal_background;
	struct vec4 background_color;
};

extern struct obs_source_info tilegen_filter;

#ifdef __cplusplus
}
#endif
