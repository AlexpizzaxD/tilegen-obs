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

	// Shape
	int shape_type;
	float shape_size;
	float edge_softness;
	float rotation_deg;
	float auto_rotate_speed;
	int shape_sides;
	float star_sharpness;

	// Mix
	int shape_type_b;
	float shape_mix_chance;

	// Image / source
	int use_source_instead_of_image;
	char *image_path;
	char *source_name;
	float image_aspect_override;

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

	// Variation
	float visibility_chance;
	float size_variation;
	float rotation_variation;
	float hue_variation;
	float twinkle_amount;
	float twinkle_speed;

	// Movement
	struct vec2 scroll_speed;
	struct vec2 scroll_offset;
	float drift_amount;
	float drift_speed;
	float pulse_amount;
	float pulse_speed;
};

extern struct obs_source_info tilegen_filter;

#ifdef __cplusplus
}
#endif
