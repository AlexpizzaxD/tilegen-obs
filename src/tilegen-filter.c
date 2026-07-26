/*
TileGen OBS
Copyright (C) 2026 LaViduka

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#include <obs-module.h>
#include <util/platform.h>
#include <plugin-support.h>
#include "tilegen-filter.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static void bleed_alpha(uint8_t *data, int w, int h, int passes)
{
	size_t size = (size_t)w * h * 4;
	uint8_t *tmp = bmalloc(size);
	for (int p = 0; p < passes; p++) {
		memcpy(tmp, data, size);
		for (int y = 0; y < h; y++) {
			for (int x = 0; x < w; x++) {
				size_t i = ((size_t)y * w + x) * 4;
				if (tmp[i + 3] != 0)
					continue;
				int rs = 0, gs = 0, bs = 0, count = 0;
				int dx[4] = {-1, 1, 0, 0};
				int dy[4] = {0, 0, -1, 1};
				for (int k = 0; k < 4; k++) {
					int nx = x + dx[k];
					int ny = y + dy[k];
					if (nx < 0 || nx >= w || ny < 0 ||
					    ny >= h)
						continue;
					size_t ni =
						((size_t)ny * w + nx) * 4;
					if (tmp[ni + 3] == 0)
						continue;
					rs += tmp[ni];
					gs += tmp[ni + 1];
					bs += tmp[ni + 2];
					count++;
				}
				if (count > 0) {
					data[i] = (uint8_t)(rs / count);
					data[i + 1] =
						(uint8_t)(gs / count);
					data[i + 2] =
						(uint8_t)(bs / count);
				}
			}
		}
	}
	bfree(tmp);
}

#define SETTING_GRID_PATTERN "grid_pattern"
#define SETTING_DENSITY "density"
#define SETTING_SQUARE_CELLS "square_cells"
#define SETTING_GAP "gap"
#define SETTING_SEED "seed"
#define SETTING_PATTERN_SCALE "pattern_scale"
#define SETTING_PATTERN_OFFSET_X "pattern_offset_x"
#define SETTING_PATTERN_OFFSET_Y "pattern_offset_y"

#define SETTING_SHAPE_TYPE "shape_type"
#define SETTING_SHAPE_SIZE "shape_size"
#define SETTING_EDGE_SOFTNESS "edge_softness"
#define SETTING_ROTATION_DEG "rotation_deg"
#define SETTING_AUTO_ROTATE_SPEED "auto_rotate_speed"
#define SETTING_SHAPE_SIDES "shape_sides"
#define SETTING_STAR_SHARPNESS "star_sharpness"
#define SETTING_STROKE_WIDTH "stroke_width"
#define SETTING_STROKE_COLOUR "stroke_colour"

#define SETTING_LINE_THICKNESS "line_thickness"
#define SETTING_ROUNDED_SQUARE_RADIUS "rounded_square_radius"

#define SETTING_SHAPE_TYPE_B "shape_type_b"
#define SETTING_SHAPE_MIX_CHANCE "shape_mix_chance"
#define SETTING_SHAPE_MIX_BLEND_WIDTH "shape_mix_blend_width"
#define SETTING_SHAPE_MIX_INDEPENDENT_MOTION "shape_mix_independent_motion"

#define SETTING_USE_SOURCE "use_source_instead_of_image"
#define SETTING_IMAGE_PATH "image_path"
#define SETTING_SOURCE_NAME "source_name"
#define SETTING_IMAGE_ASPECT "image_aspect_override"

#define SETTING_FONT_ATLAS "font_atlas"
#define SETTING_FONT_COLS "font_cols"
#define SETTING_FONT_ROWS "font_rows"
#define SETTING_FONT_INDEX "font_index"
#define SETTING_FONT_ASPECT "font_aspect_override"

#define SETTING_BASE_COLOR "base_color"
#define SETTING_SECONDARY_COLOR "secondary_color"
#define SETTING_COLOR_MODE "color_mode"
#define SETTING_COLOR_MIX "color_mix"

#define SETTING_PALETTE_COUNT "palette_count"
#define SETTING_COLOR_B "color_b"
#define SETTING_COLOR_C "color_c"
#define SETTING_COLOR_D "color_d"

#define SETTING_VISIBILITY "visibility_chance"
#define SETTING_SIZE_VARIATION "size_variation"
#define SETTING_ROTATION_VARIATION "rotation_variation"
#define SETTING_HUE_VARIATION "hue_variation"
#define SETTING_COLOR_JITTER "color_jitter"
#define SETTING_TWINKLE_AMOUNT "twinkle_amount"
#define SETTING_TWINKLE_SPEED "twinkle_speed"

#define SETTING_SCROLL_SPEED "scroll_speed"
#define SETTING_SCROLL_ANGLE "scroll_angle"
#define SETTING_DRIFT_AMOUNT "drift_amount"
#define SETTING_DRIFT_SPEED "drift_speed"
#define SETTING_PULSE_AMOUNT "pulse_amount"
#define SETTING_PULSE_SPEED "pulse_speed"

#define SETTING_WAVE_AMOUNT "wave_amount"
#define SETTING_WAVE_FREQUENCY "wave_frequency"
#define SETTING_WAVE_SPEED "wave_speed"
#define SETTING_ORBIT_AMOUNT "orbit_amount"
#define SETTING_ORBIT_SPEED "orbit_speed"

#define SETTING_GRADIENT_AXIS "gradient_axis"
#define SETTING_GRADIENT_SIZE_AMOUNT "gradient_size_amount"
#define SETTING_GRADIENT_HUE_AMOUNT "gradient_hue_amount"

#define SETTING_VIGNETTE_AMOUNT "vignette_amount"
#define SETTING_VIGNETTE_SIZE "vignette_size"
#define SETTING_VIGNETTE_SOFTNESS "vignette_softness"
#define SETTING_GLOW_AMOUNT "glow_amount"
#define SETTING_GLOW_RADIUS "glow_radius"
#define SETTING_GLOW_COLOUR "glow_colour"

#define SETTING_USE_INTERNAL_BG "use_internal_background"
#define SETTING_BACKGROUND_COLOR "background_color"

#define SETTING_PREDEFINED_PRESET "predefined_preset"

#define SETTING_IMAGE_ROTATION "image_rotation"
#define SETTING_IMAGE_SCALE "image_scale"
#define SETTING_IMAGE_OFFSET_X "image_offset_x"
#define SETTING_IMAGE_OFFSET_Y "image_offset_y"

#define SETTING_IMAGE_FIT_MODE "image_fit_mode"
#define SETTING_IMAGE_USE_SHAPE_MASK "image_use_shape_mask"
#define SETTING_IMAGE_MASK_SHAPE "image_mask_shape"

#define SETTING_RENDER_SCALE "render_scale"

struct tilegen_filter {
	obs_source_t *source;
	gs_effect_t *effect;
	struct tilegen_settings settings;
	uint64_t start_time;

	gs_texture_t *image_tex;
	gs_texrender_t *source_texr;
	gs_texrender_t *filter_texr;
	obs_source_t *source_tex_src;
	char *loaded_image_path;
	char *loaded_source_name;
};

static const char *tilegen_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("TileGen");
}

static void tilegen_update(void *data, obs_data_t *settings)
{
	struct tilegen_filter *tf = data;
	if (!tf || !settings) {
		obs_log(LOG_WARNING, "TileGen: tilegen_update called with NULL");
		return;
	}

	tf->settings.grid_pattern =
		(int)obs_data_get_int(settings, SETTING_GRID_PATTERN);
	tf->settings.density =
		(float)obs_data_get_double(settings, SETTING_DENSITY);
	tf->settings.square_cells =
		obs_data_get_bool(settings, SETTING_SQUARE_CELLS);
	tf->settings.gap = (float)obs_data_get_double(settings, SETTING_GAP);
	tf->settings.seed = (float)obs_data_get_double(settings, SETTING_SEED);
	tf->settings.pattern_scale =
		(float)obs_data_get_double(settings, SETTING_PATTERN_SCALE);
	tf->settings.pattern_offset_x =
		(float)obs_data_get_double(settings, SETTING_PATTERN_OFFSET_X);
	tf->settings.pattern_offset_y =
		(float)obs_data_get_double(settings, SETTING_PATTERN_OFFSET_Y);

	tf->settings.shape_type =
		(int)obs_data_get_int(settings, SETTING_SHAPE_TYPE);
	tf->settings.shape_size =
		(float)obs_data_get_double(settings, SETTING_SHAPE_SIZE);
	tf->settings.edge_softness =
		(float)obs_data_get_double(settings, SETTING_EDGE_SOFTNESS);
	tf->settings.rotation_deg =
		(float)obs_data_get_double(settings, SETTING_ROTATION_DEG);
	tf->settings.auto_rotate_speed =
		(float)obs_data_get_double(settings, SETTING_AUTO_ROTATE_SPEED);
	tf->settings.shape_sides =
		(int)obs_data_get_int(settings, SETTING_SHAPE_SIDES);
	tf->settings.star_sharpness =
		(float)obs_data_get_double(settings, SETTING_STAR_SHARPNESS);
	tf->settings.stroke_width =
		(float)obs_data_get_double(settings, SETTING_STROKE_WIDTH);
	uint32_t stroke = (uint32_t)obs_data_get_int(settings,
						    SETTING_STROKE_COLOUR);
	vec4_from_rgba(&tf->settings.stroke_color, stroke);

	tf->settings.line_thickness =
		(float)obs_data_get_double(settings, SETTING_LINE_THICKNESS);
	tf->settings.rounded_square_radius =
		(float)obs_data_get_double(settings,
					   SETTING_ROUNDED_SQUARE_RADIUS);

	tf->settings.shape_type_b =
		(int)obs_data_get_int(settings, SETTING_SHAPE_TYPE_B);
	tf->settings.shape_mix_chance =
		(float)obs_data_get_double(settings, SETTING_SHAPE_MIX_CHANCE);
	tf->settings.shape_mix_blend_width =
		(float)obs_data_get_double(settings,
					   SETTING_SHAPE_MIX_BLEND_WIDTH);
	tf->settings.shape_mix_independent_motion = obs_data_get_bool(
		settings, SETTING_SHAPE_MIX_INDEPENDENT_MOTION);

	tf->settings.use_source_instead_of_image =
		(int)obs_data_get_int(settings, SETTING_USE_SOURCE);
	tf->settings.image_aspect_override =
		(float)obs_data_get_double(settings, SETTING_IMAGE_ASPECT);

	const char *new_image_path =
		obs_data_get_string(settings, SETTING_IMAGE_PATH);
	if (!new_image_path)
		new_image_path = "";

	if (!tf->loaded_image_path || strcmp(tf->loaded_image_path,
					     new_image_path) != 0) {
		bfree(tf->loaded_image_path);
		tf->loaded_image_path = bstrdup(new_image_path);

		if (tf->image_tex) {
			obs_enter_graphics();
			gs_texture_destroy(tf->image_tex);
			obs_leave_graphics();
			tf->image_tex = NULL;
		}

		if (new_image_path && new_image_path[0] != '\0' &&
		    os_file_exists(new_image_path)) {
			int img_w, img_h, img_channels;
			unsigned char *img_data = stbi_load(
				new_image_path, &img_w, &img_h,
				&img_channels, 4);
			if (img_data) {
				bleed_alpha(img_data, img_w, img_h, 4);
				obs_enter_graphics();
				const uint8_t *tex_data[] = {img_data};
				tf->image_tex = gs_texture_create(
					img_w, img_h, GS_RGBA, 1,
					tex_data, 0);
				obs_leave_graphics();
				stbi_image_free(img_data);
				if (!tf->image_tex) {
					obs_log(LOG_WARNING,
						"TileGen: failed to create texture from '%s'",
						new_image_path);
				}
			} else {
				obs_log(LOG_WARNING,
					"TileGen: failed to load image '%s'",
					new_image_path);
			}
		}
	}

	const char *new_source_name =
		obs_data_get_string(settings, SETTING_SOURCE_NAME);
	if (!new_source_name)
		new_source_name = "";

	if (!tf->loaded_source_name || strcmp(tf->loaded_source_name,
					      new_source_name) != 0) {
		bfree(tf->loaded_source_name);
		tf->loaded_source_name = bstrdup(new_source_name);

		if (tf->source_tex_src) {
			obs_source_release(tf->source_tex_src);
			tf->source_tex_src = NULL;
		}

		if (new_source_name && new_source_name[0] != '\0') {
			tf->source_tex_src =
				obs_get_source_by_name(new_source_name);
		}
	}

	tf->settings.font_cols =
		(int)obs_data_get_int(settings, SETTING_FONT_COLS);
	tf->settings.font_rows =
		(int)obs_data_get_int(settings, SETTING_FONT_ROWS);
	tf->settings.font_index =
		(int)obs_data_get_int(settings, SETTING_FONT_INDEX);
	tf->settings.font_aspect_override =
		(float)obs_data_get_double(settings, SETTING_FONT_ASPECT);

	uint32_t base = (uint32_t)obs_data_get_int(settings,
						   SETTING_BASE_COLOR);
	vec4_from_rgba(&tf->settings.base_color, base);

	uint32_t secondary = (uint32_t)obs_data_get_int(
		settings, SETTING_SECONDARY_COLOR);
	vec4_from_rgba(&tf->settings.secondary_color, secondary);

	tf->settings.color_mode =
		(int)obs_data_get_int(settings, SETTING_COLOR_MODE);
	tf->settings.color_mix =
		(float)obs_data_get_double(settings, SETTING_COLOR_MIX);

	tf->settings.palette_count =
		(int)obs_data_get_int(settings, SETTING_PALETTE_COUNT);
	uint32_t cb = (uint32_t)obs_data_get_int(settings, SETTING_COLOR_B);
	vec4_from_rgba(&tf->settings.color_b, cb);
	uint32_t cc = (uint32_t)obs_data_get_int(settings, SETTING_COLOR_C);
	vec4_from_rgba(&tf->settings.color_c, cc);
	uint32_t cd = (uint32_t)obs_data_get_int(settings, SETTING_COLOR_D);
	vec4_from_rgba(&tf->settings.color_d, cd);

	tf->settings.visibility_chance =
		(float)obs_data_get_double(settings, SETTING_VISIBILITY);
	tf->settings.size_variation =
		(float)obs_data_get_double(settings, SETTING_SIZE_VARIATION);
	tf->settings.rotation_variation =
		(float)obs_data_get_double(settings, SETTING_ROTATION_VARIATION);
	tf->settings.hue_variation =
		(float)obs_data_get_double(settings, SETTING_HUE_VARIATION);
	tf->settings.color_jitter =
		(float)obs_data_get_double(settings, SETTING_COLOR_JITTER);
	tf->settings.twinkle_amount =
		(float)obs_data_get_double(settings, SETTING_TWINKLE_AMOUNT);
	tf->settings.twinkle_speed =
		(float)obs_data_get_double(settings, SETTING_TWINKLE_SPEED);

	tf->settings.scroll_speed =
		(float)obs_data_get_double(settings, SETTING_SCROLL_SPEED);
	tf->settings.scroll_angle =
		(float)obs_data_get_double(settings, SETTING_SCROLL_ANGLE);
	tf->settings.drift_amount =
		(float)obs_data_get_double(settings, SETTING_DRIFT_AMOUNT);
	tf->settings.drift_speed =
		(float)obs_data_get_double(settings, SETTING_DRIFT_SPEED);
	tf->settings.pulse_amount =
		(float)obs_data_get_double(settings, SETTING_PULSE_AMOUNT);
	tf->settings.pulse_speed =
		(float)obs_data_get_double(settings, SETTING_PULSE_SPEED);

	tf->settings.wave_amount =
		(float)obs_data_get_double(settings, SETTING_WAVE_AMOUNT);
	tf->settings.wave_frequency =
		(float)obs_data_get_double(settings, SETTING_WAVE_FREQUENCY);
	tf->settings.wave_speed =
		(float)obs_data_get_double(settings, SETTING_WAVE_SPEED);
	tf->settings.orbit_amount =
		(float)obs_data_get_double(settings, SETTING_ORBIT_AMOUNT);
	tf->settings.orbit_speed =
		(float)obs_data_get_double(settings, SETTING_ORBIT_SPEED);

	tf->settings.gradient_axis =
		(int)obs_data_get_int(settings, SETTING_GRADIENT_AXIS);
	tf->settings.gradient_size_amount =
		(float)obs_data_get_double(settings,
					   SETTING_GRADIENT_SIZE_AMOUNT);
	tf->settings.gradient_hue_amount =
		(float)obs_data_get_double(settings,
					   SETTING_GRADIENT_HUE_AMOUNT);

	tf->settings.vignette_amount =
		(float)obs_data_get_double(settings, SETTING_VIGNETTE_AMOUNT);
	tf->settings.vignette_size =
		(float)obs_data_get_double(settings, SETTING_VIGNETTE_SIZE);
	tf->settings.vignette_softness =
		(float)obs_data_get_double(settings,
					   SETTING_VIGNETTE_SOFTNESS);

	tf->settings.glow_amount =
		(float)obs_data_get_double(settings, SETTING_GLOW_AMOUNT);
	tf->settings.glow_radius =
		(float)obs_data_get_double(settings, SETTING_GLOW_RADIUS);
	uint32_t glow = (uint32_t)obs_data_get_int(settings,
						  SETTING_GLOW_COLOUR);
	vec4_from_rgba(&tf->settings.glow_color, glow);

	tf->settings.image_rotation =
		(float)obs_data_get_double(settings,
					  SETTING_IMAGE_ROTATION);
	tf->settings.image_scale =
		(float)obs_data_get_double(settings,
					  SETTING_IMAGE_SCALE);
	tf->settings.image_offset_x =
		(float)obs_data_get_double(settings,
					  SETTING_IMAGE_OFFSET_X);
	tf->settings.image_offset_y =
		(float)obs_data_get_double(settings,
					  SETTING_IMAGE_OFFSET_Y);

	tf->settings.image_fit_mode =
		(int)obs_data_get_int(settings, SETTING_IMAGE_FIT_MODE);
	tf->settings.image_use_shape_mask = obs_data_get_bool(
		settings, SETTING_IMAGE_USE_SHAPE_MASK);
	tf->settings.image_mask_shape =
		(int)obs_data_get_int(settings, SETTING_IMAGE_MASK_SHAPE);

	tf->settings.render_scale =
		(float)obs_data_get_double(settings, SETTING_RENDER_SCALE);

	tf->settings.predefined_preset =
		(int)obs_data_get_int(settings, SETTING_PREDEFINED_PRESET);

	tf->settings.use_internal_background =
		(int)obs_data_get_int(settings, SETTING_USE_INTERNAL_BG);
	uint32_t bg = (uint32_t)obs_data_get_int(settings,
						 SETTING_BACKGROUND_COLOR);
	vec4_from_rgba(&tf->settings.background_color, bg);
}

static void *tilegen_create(obs_data_t *settings, obs_source_t *source)
{
	struct tilegen_filter *tf = bzalloc(sizeof(struct tilegen_filter));
	if (!tf) {
		obs_log(LOG_ERROR, "TileGen: failed to allocate filter struct");
		return NULL;
	}
	tf->source = source;
	tf->start_time = obs_get_video_frame_time();

	char *effect_path = obs_module_file("effects/tilegen.effect");
	if (effect_path) {
		obs_enter_graphics();

		char *error = NULL;
		tf->effect = gs_effect_create_from_file(effect_path, &error);
		if (!tf->effect && error) {
			obs_log(LOG_ERROR, "Failed to load tilegen.effect: %s",
				error);
			bfree(error);
		} else if (!tf->effect) {
			obs_log(LOG_ERROR,
				"Failed to load tilegen.effect (unknown error)");
		}

		obs_leave_graphics();

		bfree(effect_path);
	} else {
		obs_log(LOG_ERROR, "TileGen: could not find effect path");
	}

	tilegen_update(tf, settings);
	return tf;
}

static void tilegen_destroy(void *data)
{
	struct tilegen_filter *tf = data;

	bfree(tf->settings.image_path);
	bfree(tf->settings.source_name);
	bfree(tf->settings.font_atlas_path);

	if (tf->effect || tf->image_tex || tf->source_texr ||
	    tf->filter_texr) {
		obs_enter_graphics();
		if (tf->source_texr)
			gs_texrender_destroy(tf->source_texr);
		if (tf->filter_texr)
			gs_texrender_destroy(tf->filter_texr);
		gs_texture_destroy(tf->image_tex);
		if (tf->effect)
			gs_effect_destroy(tf->effect);
		obs_leave_graphics();
	}
	bfree(tf->loaded_image_path);
	bfree(tf->loaded_source_name);
	if (tf->source_tex_src)
		obs_source_release(tf->source_tex_src);
	bfree(tf);
}

static void set_uniform_int(gs_effect_t *effect, const char *name, int value)
{
	gs_eparam_t *param = gs_effect_get_param_by_name(effect, name);
	if (param)
		gs_effect_set_int(param, value);
}

static void set_uniform_float(gs_effect_t *effect, const char *name,
			      float value)
{
	gs_eparam_t *param = gs_effect_get_param_by_name(effect, name);
	if (param)
		gs_effect_set_float(param, value);
}

static void set_uniform_bool(gs_effect_t *effect, const char *name,
			     bool value)
{
	gs_eparam_t *param = gs_effect_get_param_by_name(effect, name);
	if (param)
		gs_effect_set_bool(param, value);
}

static void set_uniform_vec2(gs_effect_t *effect, const char *name,
			     const struct vec2 *value)
{
	gs_eparam_t *param = gs_effect_get_param_by_name(effect, name);
	if (param)
		gs_effect_set_vec2(param, value);
}

static void set_uniform_texture(gs_effect_t *effect, const char *name,
				gs_texture_t *tex)
{
	gs_eparam_t *param = gs_effect_get_param_by_name(effect, name);
	if (param)
		gs_effect_set_texture(param, tex);
}

static bool add_source_to_list(void *data, obs_source_t *source)
{
	obs_property_t *list = (obs_property_t *)data;
	uint32_t caps = obs_source_get_output_flags(source);
	if (caps & OBS_SOURCE_VIDEO) {
		const char *name = obs_source_get_name(source);
		const char *id = obs_source_get_id(source);
		const char *type_name = obs_source_get_display_name(id);
		char label[256];
		if (type_name && *type_name)
			snprintf(label, sizeof(label), "[%s] %s", type_name,
				 name);
		else
			snprintf(label, sizeof(label), "%s", name);
		obs_property_list_add_string(list, label, name);
	}
	return true;
}

/* ============================================================
 * Predefined pattern presets
 * ============================================================ */

struct preset_def {
	const char *name;
	int grid_pattern;
	float density;
	int shape_type;
	int shape_type_b;
	float shape_mix_chance;
	float size_variation;
	float pulse_amount;
	float pulse_speed;
	float drift_amount;
	float glow_amount;
	float vignette_amount;
	int color_mode;
	int shape_sides;
};

static const struct preset_def PRESETS[] = {
	{ "Default",       0, 20.0f,  0,  6, 0.0f, 0.0f, 0.0f, 1.0f,
	  0.0f, 0.0f, 0.0f, 0, 5 },
	{ "HexDots",       2, 15.0f,  0,  0, 0.0f, 0.0f, 0.0f, 1.0f,
	  0.0f, 0.0f, 0.0f, 0, 5 },
	{ "BrickLines",    1, 30.0f, 12, 13, 0.5f, 0.0f, 0.0f, 1.0f,
	  0.0f, 0.0f, 0.0f, 0, 5 },
	{ "StarField",     0, 20.0f,  6,  0, 0.0f, 0.3f, 0.0f, 1.0f,
	  0.0f, 0.2f, 0.0f, 0, 5 },
	{ "Confetti",      0, 18.0f,  0,  6, 0.5f, 0.4f, 0.0f, 1.0f,
	  0.0f, 0.0f, 0.0f, 1, 5 },
	{ "WaveDots",      0, 15.0f,  0,  0, 0.0f, 0.0f, 0.0f, 1.0f,
	  0.5f, 0.0f, 0.0f, 0, 5 },
	{ "GlowingHearts", 0, 12.0f,  9,  0, 0.0f, 0.2f, 0.4f, 0.8f,
	  0.0f, 0.6f, 0.0f, 0, 5 },
	{ "VignetteFrame", 0,  8.0f, 11,  0, 0.0f, 0.0f, 0.0f, 1.0f,
	  0.0f, 0.0f, 0.4f, 0, 5 },
	{ "CrossesGrid",   0, 15.0f, 10,  0, 0.0f, 0.0f, 0.0f, 1.0f,
	  0.0f, 0.0f, 0.0f, 0, 5 },
	{ "PulseCircles",  0, 12.0f,  0,  0, 0.0f, 0.0f, 0.5f, 2.0f,
	  0.0f, 0.0f, 0.0f, 0, 5 },
	{ "Triangles",     0, 18.0f,  8,  0, 0.0f, 0.2f, 0.0f, 1.0f,
	  0.0f, 0.0f, 0.0f, 0, 5 },
	{ "Honeycomb",     2, 12.0f,  5,  0, 0.0f, 0.0f, 0.0f, 1.0f,
	  0.0f, 0.0f, 0.0f, 2, 6 },
};

static const size_t PRESETS_COUNT =
	sizeof(PRESETS) / sizeof(PRESETS[0]);

static const char *predefined_locale_key(int idx)
{
	static const char *keys[] = {
		"PredefDefault",      "PredefHexDots",
		"PredefBrickLines",   "PredefStarField",
		"PredefConfetti",     "PredefWaveDots",
		"PredefGlowingHearts", "PredefVignetteFrame",
		"PredefCrossesGrid",  "PredefPulseCircles",
		"PredefTriangles",    "PredefHoneycomb",
	};
	if (idx < 0 ||
	    idx >= (int)(sizeof(keys) / sizeof(keys[0])))
		return "PredefDefault";
	return keys[idx];
}

static bool random_seed_callback(obs_properties_t *props, obs_property_t *property,
				void *data)
{
	UNUSED_PARAMETER(property);
	UNUSED_PARAMETER(props);
	struct tilegen_filter *tf = data;
	if (!tf)
		return false;
	double new_seed = (double)(rand() % 10001) / 100.0;
	obs_data_t *settings = obs_source_get_settings(tf->source);
	obs_data_set_double(settings, SETTING_SEED, new_seed);
	obs_source_update(tf->source, settings);
	obs_data_release(settings);
	return true;
}

static bool predefined_preset_callback(obs_properties_t *props,
					obs_property_t *property, void *data)
{
	UNUSED_PARAMETER(property);
	UNUSED_PARAMETER(props);
	obs_data_t *settings = data;
	if (!settings)
		return false;

	int idx = (int)obs_data_get_int(settings, SETTING_PREDEFINED_PRESET);

	if (idx < 0 || (size_t)idx >= PRESETS_COUNT)
		idx = 0;

	const struct preset_def *p = &PRESETS[idx];
	obs_data_set_int(settings, SETTING_GRID_PATTERN, p->grid_pattern);
	obs_data_set_double(settings, SETTING_DENSITY, p->density);
	obs_data_set_int(settings, SETTING_SHAPE_TYPE, p->shape_type);
	obs_data_set_int(settings, SETTING_SHAPE_TYPE_B, p->shape_type_b);
	obs_data_set_double(settings, SETTING_SHAPE_MIX_CHANCE,
			  p->shape_mix_chance);
	obs_data_set_double(settings, SETTING_SIZE_VARIATION,
			  p->size_variation);
	obs_data_set_double(settings, SETTING_PULSE_AMOUNT, p->pulse_amount);
	obs_data_set_double(settings, SETTING_PULSE_SPEED, p->pulse_speed);
	obs_data_set_double(settings, SETTING_DRIFT_AMOUNT, p->drift_amount);
	obs_data_set_double(settings, SETTING_GLOW_AMOUNT, p->glow_amount);
	obs_data_set_double(settings, SETTING_VIGNETTE_AMOUNT,
			  p->vignette_amount);
	obs_data_set_int(settings, SETTING_COLOR_MODE, p->color_mode);
	obs_data_set_int(settings, SETTING_SHAPE_SIDES, p->shape_sides);

	return true;
}

static bool reset_to_defaults_callback(obs_properties_t *props,
				       obs_property_t *property, void *data)
{
	UNUSED_PARAMETER(property);
	UNUSED_PARAMETER(props);
	obs_log(LOG_INFO,
		"TileGen: reset_to_defaults_callback called (data=%p)",
		data);
	struct tilegen_filter *tf = data;
	if (!tf) {
		obs_log(LOG_WARNING,
			"TileGen: reset_to_defaults_callback: tf is NULL");
		return false;
	}

	obs_data_t *settings = obs_source_get_settings(tf->source);

	// Grid
	obs_data_set_int(settings, SETTING_GRID_PATTERN, 0);
	obs_data_set_double(settings, SETTING_DENSITY, 20.0);
	obs_data_set_bool(settings, SETTING_SQUARE_CELLS, false);
	obs_data_set_double(settings, SETTING_GAP, 0.0);
	obs_data_set_double(settings, SETTING_SEED, 0.0);
	obs_data_set_double(settings, SETTING_PATTERN_SCALE, 1.0);
	obs_data_set_double(settings, SETTING_PATTERN_OFFSET_X, 0.0);
	obs_data_set_double(settings, SETTING_PATTERN_OFFSET_Y, 0.0);

	// Shape
	obs_data_set_int(settings, SETTING_SHAPE_TYPE, 0);
	obs_data_set_double(settings, SETTING_SHAPE_SIZE, 0.5);
	obs_data_set_double(settings, SETTING_EDGE_SOFTNESS, 0.02);
	obs_data_set_double(settings, SETTING_ROTATION_DEG, 0.0);
	obs_data_set_double(settings, SETTING_AUTO_ROTATE_SPEED, 0.0);
	obs_data_set_int(settings, SETTING_SHAPE_SIDES, 5);
	obs_data_set_double(settings, SETTING_STAR_SHARPNESS, 3.0);
	obs_data_set_double(settings, SETTING_STROKE_WIDTH, 0.0);
	obs_data_set_int(settings, SETTING_STROKE_COLOUR, 0xFFFFFFFF);

	obs_data_set_double(settings, SETTING_LINE_THICKNESS, 0.05);
	obs_data_set_double(settings, SETTING_ROUNDED_SQUARE_RADIUS, 0.15);

	// Mix
	obs_data_set_int(settings, SETTING_SHAPE_TYPE_B, 6);
	obs_data_set_double(settings, SETTING_SHAPE_MIX_CHANCE, 0.0);
	obs_data_set_double(settings, SETTING_SHAPE_MIX_BLEND_WIDTH, 0.0);
	obs_data_set_bool(settings, SETTING_SHAPE_MIX_INDEPENDENT_MOTION,
			  true);

	// Image / Source
	obs_data_set_int(settings, SETTING_USE_SOURCE, 0);
	obs_data_set_double(settings, SETTING_IMAGE_ASPECT, 0.0);
	obs_data_set_string(settings, SETTING_IMAGE_PATH, "");
	obs_data_set_string(settings, SETTING_SOURCE_NAME, "");

	// Color
	obs_data_set_int(settings, SETTING_BASE_COLOR, 0xFFFFFFFF);
	obs_data_set_int(settings, SETTING_SECONDARY_COLOR, 0xFFFF0000);
	obs_data_set_int(settings, SETTING_COLOR_MODE, 0);
	obs_data_set_double(settings, SETTING_COLOR_MIX, 0.0);

	obs_data_set_int(settings, SETTING_PALETTE_COUNT, 1);
	obs_data_set_int(settings, SETTING_COLOR_B, 0xFFFF0000);
	obs_data_set_int(settings, SETTING_COLOR_C, 0xFF00FF00);
	obs_data_set_int(settings, SETTING_COLOR_D, 0xFF0000FF);

	// Variation
	obs_data_set_double(settings, SETTING_VISIBILITY, 1.0);
	obs_data_set_double(settings, SETTING_SIZE_VARIATION, 0.0);
	obs_data_set_double(settings, SETTING_ROTATION_VARIATION, 0.0);
	obs_data_set_double(settings, SETTING_HUE_VARIATION, 0.0);
	obs_data_set_double(settings, SETTING_COLOR_JITTER, 0.0);
	obs_data_set_double(settings, SETTING_TWINKLE_AMOUNT, 0.0);
	obs_data_set_double(settings, SETTING_TWINKLE_SPEED, 1.0);

	// Motion
	obs_data_set_double(settings, SETTING_SCROLL_SPEED, 0.0);
	obs_data_set_double(settings, SETTING_SCROLL_ANGLE, 0.0);
	obs_data_set_double(settings, SETTING_DRIFT_AMOUNT, 0.0);
	obs_data_set_double(settings, SETTING_DRIFT_SPEED, 1.0);
	obs_data_set_double(settings, SETTING_PULSE_AMOUNT, 0.0);
	obs_data_set_double(settings, SETTING_PULSE_SPEED, 1.0);

	obs_data_set_double(settings, SETTING_WAVE_AMOUNT, 0.0);
	obs_data_set_double(settings, SETTING_WAVE_FREQUENCY, 4.0);
	obs_data_set_double(settings, SETTING_WAVE_SPEED, 1.0);
	obs_data_set_double(settings, SETTING_ORBIT_AMOUNT, 0.0);
	obs_data_set_double(settings, SETTING_ORBIT_SPEED, 30.0);

	obs_data_set_int(settings, SETTING_GRADIENT_AXIS, 0);
	obs_data_set_double(settings, SETTING_GRADIENT_SIZE_AMOUNT, 0.0);
	obs_data_set_double(settings, SETTING_GRADIENT_HUE_AMOUNT, 0.0);

	// Glow + Vignette
	obs_data_set_double(settings, SETTING_GLOW_AMOUNT, 0.0);
	obs_data_set_double(settings, SETTING_GLOW_RADIUS, 0.1);
	obs_data_set_int(settings, SETTING_GLOW_COLOUR, 0xFFFFFFFF);
	obs_data_set_double(settings, SETTING_VIGNETTE_AMOUNT, 0.0);
	obs_data_set_double(settings, SETTING_VIGNETTE_SIZE, 0.5);
	obs_data_set_double(settings, SETTING_VIGNETTE_SOFTNESS, 0.5);

	// Background
	obs_data_set_int(settings, SETTING_USE_INTERNAL_BG, 1);
	obs_data_set_int(settings, SETTING_BACKGROUND_COLOR, 0xFF000000);

	obs_data_set_int(settings, SETTING_IMAGE_FIT_MODE, 0);
	obs_data_set_bool(settings, SETTING_IMAGE_USE_SHAPE_MASK, false);
	obs_data_set_int(settings, SETTING_IMAGE_MASK_SHAPE, 0);

	obs_source_update(tf->source, settings);
	obs_data_release(settings);
	obs_log(LOG_INFO, "TileGen: reset_to_defaults_callback completed");
	return true;
}

static void set_uniform_vec4(gs_effect_t *effect, const char *name,
			     const struct vec4 *value)
{
	gs_eparam_t *param = gs_effect_get_param_by_name(effect, name);
	if (param)
		gs_effect_set_vec4(param, value);
}

static void tilegen_video_render(void *data, gs_effect_t *effect)
{
	struct tilegen_filter *tf = data;
	if (!tf || !tf->source) {
		obs_log(LOG_WARNING, "TileGen: render with NULL data");
		return;
	}

	obs_source_t *target = obs_filter_get_target(tf->source);
	if (!target) {
		obs_source_skip_video_filter(tf->source);
		return;
	}

	if (!obs_source_process_filter_begin(tf->source, GS_RGBA,
					     OBS_ALLOW_DIRECT_RENDERING)) {
		return;
	}

	if (tf->effect) {
		uint64_t now = obs_get_video_frame_time();
		float elapsed_time =
			(float)((now - tf->start_time) / 1000000000.0);

		struct vec2 uv_size;
		uv_size.x = (float)obs_source_get_base_width(target);
		uv_size.y = (float)obs_source_get_base_height(target);

		set_uniform_float(tf->effect, "elapsed_time", elapsed_time);
		set_uniform_vec2(tf->effect, "uv_size", &uv_size);

		set_uniform_int(tf->effect, "grid_pattern",
				tf->settings.grid_pattern);
		set_uniform_float(tf->effect, "density",
				  tf->settings.density);
		set_uniform_bool(tf->effect, "square_cells",
				 tf->settings.square_cells);
		set_uniform_float(tf->effect, "gap", tf->settings.gap);
		set_uniform_float(tf->effect, "seed", tf->settings.seed);
		set_uniform_float(tf->effect, "pattern_scale",
				  tf->settings.pattern_scale);
		set_uniform_float(tf->effect, "pattern_offset_x",
				  tf->settings.pattern_offset_x);
		set_uniform_float(tf->effect, "pattern_offset_y",
				  tf->settings.pattern_offset_y);

		set_uniform_int(tf->effect, "shape_type",
				tf->settings.shape_type);
		set_uniform_float(tf->effect, "shape_size",
				  tf->settings.shape_size);
		set_uniform_float(tf->effect, "edge_softness",
				  tf->settings.edge_softness);
		set_uniform_float(tf->effect, "rotation_deg",
				  tf->settings.rotation_deg);
		set_uniform_float(tf->effect, "auto_rotate_speed",
				  tf->settings.auto_rotate_speed);
		set_uniform_int(tf->effect, "shape_sides",
				tf->settings.shape_sides);
		set_uniform_float(tf->effect, "star_sharpness",
				  tf->settings.star_sharpness);
		set_uniform_float(tf->effect, "stroke_width",
				  tf->settings.stroke_width);
		set_uniform_vec4(tf->effect, "stroke_color",
				 &tf->settings.stroke_color);

		set_uniform_float(tf->effect, "line_thickness",
				  tf->settings.line_thickness);
		set_uniform_float(tf->effect, "rounded_square_radius",
				  tf->settings.rounded_square_radius);

		set_uniform_int(tf->effect, "shape_type_b",
				tf->settings.shape_type_b);
		set_uniform_float(tf->effect, "shape_mix_chance",
				  tf->settings.shape_mix_chance);
		set_uniform_float(tf->effect, "shape_mix_blend_width",
				  tf->settings.shape_mix_blend_width);
		set_uniform_bool(tf->effect, "shape_mix_independent_motion",
				 tf->settings.shape_mix_independent_motion);

		set_uniform_int(tf->effect, "use_source_instead_of_image",
				tf->settings.use_source_instead_of_image);
		set_uniform_float(tf->effect, "image_aspect_override",
				  tf->settings.image_aspect_override);

		gs_texture_t *src_tex = NULL;
		if (tf->settings.use_source_instead_of_image != 0 &&
		    tf->source_tex_src) {
			if (!tf->source_texr) {
				tf->source_texr = gs_texrender_create(
					GS_RGBA, GS_ZS_NONE);
			}
			if (tf->source_texr) {
				uint32_t w = obs_source_get_width(
					tf->source_tex_src);
				uint32_t h = obs_source_get_height(
					tf->source_tex_src);
				if (w > 0 && h > 0) {
					gs_texrender_reset(tf->source_texr);
					if (gs_texrender_begin(
						    tf->source_texr, w,
						    h)) {
						obs_source_video_render(
							tf->source_tex_src);
						gs_texrender_end(
							tf->source_texr);
						src_tex =
							gs_texrender_get_texture(
								tf->source_texr);
					}
				}
			}
		}
		set_uniform_texture(tf->effect, "image_tex",
				    tf->image_tex);
		set_uniform_texture(tf->effect, "source_tex", src_tex);

		int image_or_source_loaded = 0;
		if (tf->settings.use_source_instead_of_image != 0) {
			image_or_source_loaded = (src_tex != NULL) ? 1 : 0;
		} else {
			image_or_source_loaded = (tf->image_tex != NULL) ? 1
									  : 0;
		}
		set_uniform_int(tf->effect, "image_or_source_loaded",
				image_or_source_loaded);

		set_uniform_int(tf->effect, "font_cols",
				tf->settings.font_cols);
		set_uniform_int(tf->effect, "font_rows",
				tf->settings.font_rows);
		set_uniform_int(tf->effect, "font_index",
				tf->settings.font_index);
		set_uniform_float(tf->effect, "font_aspect_override",
				  tf->settings.font_aspect_override);

		set_uniform_vec4(tf->effect, "base_color",
				 &tf->settings.base_color);
		set_uniform_vec4(tf->effect, "secondary_color",
				 &tf->settings.secondary_color);
		set_uniform_int(tf->effect, "color_mode",
				tf->settings.color_mode);
		set_uniform_float(tf->effect, "color_mix",
				  tf->settings.color_mix);

		set_uniform_int(tf->effect, "palette_count",
				tf->settings.palette_count);
		set_uniform_vec4(tf->effect, "color_b",
				 &tf->settings.color_b);
		set_uniform_vec4(tf->effect, "color_c",
				 &tf->settings.color_c);
		set_uniform_vec4(tf->effect, "color_d",
				 &tf->settings.color_d);

		set_uniform_float(tf->effect, "visibility_chance",
				  tf->settings.visibility_chance);
		set_uniform_float(tf->effect, "size_variation",
				  tf->settings.size_variation);
		set_uniform_float(tf->effect, "rotation_variation",
				  tf->settings.rotation_variation);
		set_uniform_float(tf->effect, "hue_variation",
				  tf->settings.hue_variation);
		set_uniform_float(tf->effect, "color_jitter",
				  tf->settings.color_jitter);
		set_uniform_float(tf->effect, "twinkle_amount",
				  tf->settings.twinkle_amount);
		set_uniform_float(tf->effect, "twinkle_speed",
				  tf->settings.twinkle_speed);

		set_uniform_float(tf->effect, "scroll_speed",
				  tf->settings.scroll_speed);
		set_uniform_float(tf->effect, "scroll_angle",
				  tf->settings.scroll_angle);
		set_uniform_float(tf->effect, "drift_amount",
				  tf->settings.drift_amount);
		set_uniform_float(tf->effect, "drift_speed",
				  tf->settings.drift_speed);
		set_uniform_float(tf->effect, "pulse_amount",
				  tf->settings.pulse_amount);
		set_uniform_float(tf->effect, "pulse_speed",
				  tf->settings.pulse_speed);

		set_uniform_float(tf->effect, "wave_amount",
				  tf->settings.wave_amount);
		set_uniform_float(tf->effect, "wave_frequency",
				  tf->settings.wave_frequency);
		set_uniform_float(tf->effect, "wave_speed",
				  tf->settings.wave_speed);
		set_uniform_float(tf->effect, "orbit_amount",
				  tf->settings.orbit_amount);
		set_uniform_float(tf->effect, "orbit_speed",
				  tf->settings.orbit_speed);

		set_uniform_int(tf->effect, "gradient_axis",
				tf->settings.gradient_axis);
		set_uniform_float(tf->effect, "gradient_size_amount",
				  tf->settings.gradient_size_amount);
		set_uniform_float(tf->effect, "gradient_hue_amount",
				  tf->settings.gradient_hue_amount);

		set_uniform_float(tf->effect, "vignette_amount",
				  tf->settings.vignette_amount);
		set_uniform_float(tf->effect, "vignette_size",
				  tf->settings.vignette_size);
		set_uniform_float(tf->effect, "vignette_softness",
				  tf->settings.vignette_softness);

		set_uniform_float(tf->effect, "glow_amount",
				  tf->settings.glow_amount);
		set_uniform_float(tf->effect, "glow_radius",
				  tf->settings.glow_radius);
		set_uniform_vec4(tf->effect, "glow_color",
				 &tf->settings.glow_color);

		set_uniform_float(tf->effect, "image_rotation",
				  tf->settings.image_rotation);
		set_uniform_float(tf->effect, "image_scale",
				  tf->settings.image_scale);
		set_uniform_float(tf->effect, "image_offset_x",
				  tf->settings.image_offset_x);
		set_uniform_float(tf->effect, "image_offset_y",
				  tf->settings.image_offset_y);

		set_uniform_int(tf->effect, "image_fit_mode",
				tf->settings.image_fit_mode);
		set_uniform_bool(tf->effect, "image_use_shape_mask",
				 tf->settings.image_use_shape_mask);
		set_uniform_int(tf->effect, "image_mask_shape",
				tf->settings.image_mask_shape);

		set_uniform_float(tf->effect, "render_scale",
				  tf->settings.render_scale);
		/* TODO: implement full render scale pipeline with gs_texrender_t
		 * Currently the uniform is set but the actual downscaling
		 * requires a pipeline refactor that bypasses
		 * obs_source_process_filter_begin. */

		set_uniform_int(tf->effect, "use_internal_background",
				tf->settings.use_internal_background);
		set_uniform_vec4(tf->effect, "background_color",
				 &tf->settings.background_color);

		obs_source_process_filter_end(tf->source, tf->effect, 0, 0);
	} else {
		obs_source_process_filter_end(tf->source, effect, 0, 0);
	}
}

static void add_shape_list(obs_property_t *list)
{
	obs_property_list_add_int(list, "Dot", 0);
	obs_property_list_add_int(list, "Ring", 1);
	obs_property_list_add_int(list, "Square", 2);
	obs_property_list_add_int(list, "Diamond", 3);
	obs_property_list_add_int(list, "Plus", 4);
	obs_property_list_add_int(list, "Polygon", 5);
	obs_property_list_add_int(list, "Star", 6);
	obs_property_list_add_int(list, "Image / OBS Source", 7);
	obs_property_list_add_int(list, "Triangle", 8);
	obs_property_list_add_int(list, "Heart", 9);
	obs_property_list_add_int(list, "Cross", 10);
	obs_property_list_add_int(list, "Frame", 11);
	obs_property_list_add_int(list, "Horizontal Line", 12);
	obs_property_list_add_int(list, "Vertical Line", 13);
	obs_property_list_add_int(list, "Rounded Square", 14);
	obs_property_list_add_int(list, "Cross (X)", 15);
}

static void set_desc(obs_properties_t *props, const char *name,
		    const char *locale_key)
{
	obs_property_t *p = obs_properties_get(props, name);
	if (p)
		obs_property_set_long_description(p,
						  obs_module_text(locale_key));
}

static obs_properties_t *tilegen_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();
	UNUSED_PARAMETER(data);

	/* ============================================================
	 * Predefined pattern preset
	 * ============================================================ */
	obs_property_t *predef = obs_properties_add_list(
		props, SETTING_PREDEFINED_PRESET,
		obs_module_text("PredefinedPreset"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(predef, "—", -1);
	for (size_t i = 0; i < PRESETS_COUNT; i++) {
		obs_property_list_add_int(predef,
					  obs_module_text(predefined_locale_key(
						  (int)i)),
					  (int)i);
	}
	obs_property_set_modified_callback(predef,
					   predefined_preset_callback);

	obs_properties_add_float_slider(props, SETTING_RENDER_SCALE,
					obs_module_text("RenderScale"),
					0.25, 1.0, 0.05);
	set_desc(props, SETTING_RENDER_SCALE, "DescRenderScale");

	/* ============================================================
	 * Background group
	 * ============================================================ */
	obs_properties_t *bg = obs_properties_create();
	obs_properties_add_color_alpha(bg, SETTING_BACKGROUND_COLOR,
				       obs_module_text("BackgroundColour"));
	set_desc(bg, SETTING_BACKGROUND_COLOR, "DescBackgroundColour");
	obs_properties_add_bool(bg, SETTING_USE_INTERNAL_BG,
				obs_module_text("OverrideSourceBackground"));
	set_desc(bg, SETTING_USE_INTERNAL_BG, "DescOverrideSourceBackground");
	obs_properties_add_float_slider(bg, SETTING_VIGNETTE_AMOUNT,
					obs_module_text("Vignette"), 0.0,
					1.0, 0.01);
	set_desc(bg, SETTING_VIGNETTE_AMOUNT, "DescVignette");
	obs_properties_add_float_slider(bg, SETTING_VIGNETTE_SIZE,
					obs_module_text("VignetteSize"),
					0.0, 1.5, 0.01);
	set_desc(bg, SETTING_VIGNETTE_SIZE, "DescVignetteSize");
	obs_properties_add_float_slider(bg, SETTING_VIGNETTE_SOFTNESS,
					obs_module_text("VignetteSoftness"),
					0.0, 1.0, 0.01);
	set_desc(bg, SETTING_VIGNETTE_SOFTNESS, "DescVignetteSoftness");
	obs_properties_add_float_slider(bg, SETTING_GLOW_AMOUNT,
					obs_module_text("GlowAmount"), 0.0,
					1.0, 0.01);
	set_desc(bg, SETTING_GLOW_AMOUNT, "DescGlow");
	obs_properties_add_float_slider(bg, SETTING_GLOW_RADIUS,
					obs_module_text("GlowRadius"),
					0.0, 0.5, 0.01);
	set_desc(bg, SETTING_GLOW_RADIUS, "DescGlowRadius");
	obs_properties_add_color_alpha(bg, SETTING_GLOW_COLOUR,
				       obs_module_text("GlowColour"));
	set_desc(bg, SETTING_GLOW_COLOUR, "DescGlowColour");
	obs_properties_add_group(props, "Background",
				obs_module_text("GroupBackground"),
				OBS_GROUP_NORMAL, bg);

	/* ============================================================
	 * Fuera de grupo: Shape, Mix With, OBS Source, Custom Image
	 * ============================================================ */
	obs_property_t *shape_type = obs_properties_add_list(
		props, SETTING_SHAPE_TYPE, obs_module_text("Shape"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	set_desc(props, SETTING_SHAPE_TYPE, "DescShape");
	add_shape_list(shape_type);

	obs_property_t *shape_type_b = obs_properties_add_list(
		props, SETTING_SHAPE_TYPE_B, obs_module_text("MixWith"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	set_desc(props, SETTING_SHAPE_TYPE_B, "DescMixWith");
	add_shape_list(shape_type_b);

	obs_property_t *source_list = obs_properties_add_list(
		props, SETTING_SOURCE_NAME, obs_module_text("ObsSource"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);
	set_desc(props, SETTING_SOURCE_NAME, "DescObsSource");
	obs_property_list_add_string(source_list, "", "");
	obs_enum_sources(add_source_to_list, source_list);
	obs_properties_add_bool(props, SETTING_USE_SOURCE,
			       obs_module_text("UseSourceAsTexture"));
	set_desc(props, SETTING_USE_SOURCE, "DescUseSourceAsTexture");

	obs_properties_add_path(props, SETTING_IMAGE_PATH,
				obs_module_text("CustomImage"),
				OBS_PATH_FILE,
				"Image Files (*.png *.jpg *.jpeg *.bmp);;"
				"All Files (*.*)",
				NULL);
	set_desc(props, SETTING_IMAGE_PATH, "DescCustomImage");
	obs_properties_add_float_slider(props, SETTING_IMAGE_ASPECT,
					obs_module_text("ImageAspectOverride"),
					0.0, 2.0, 0.01);
	set_desc(props, SETTING_IMAGE_ASPECT, "DescImageAspectOverride");
	obs_properties_add_float_slider(props, SETTING_IMAGE_ROTATION,
					obs_module_text("ImageRotation"),
					0.0, 360.0, 1.0);
	obs_properties_add_float_slider(props, SETTING_IMAGE_SCALE,
					obs_module_text("ImageScale"),
					0.1, 3.0, 0.01);
	obs_properties_add_float_slider(props, SETTING_IMAGE_OFFSET_X,
					obs_module_text("ImageOffsetX"),
					-1.0, 1.0, 0.01);
	obs_properties_add_float_slider(props, SETTING_IMAGE_OFFSET_Y,
					obs_module_text("ImageOffsetY"),
					-1.0, 1.0, 0.01);
	obs_property_t *fit_mode = obs_properties_add_list(
		props, SETTING_IMAGE_FIT_MODE,
		obs_module_text("ImageFitMode"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(fit_mode,
				  obs_module_text("ImageFitStretch"), 0);
	obs_property_list_add_int(fit_mode,
				  obs_module_text("ImageFitContain"), 1);
	obs_property_list_add_int(fit_mode,
				  obs_module_text("ImageFitCover"), 2);
	set_desc(props, SETTING_IMAGE_FIT_MODE, "DescImageFitMode");
	obs_properties_add_bool(
		props, SETTING_IMAGE_USE_SHAPE_MASK,
		obs_module_text("ImageUseShapeMask"));
	set_desc(props, SETTING_IMAGE_USE_SHAPE_MASK,
		 "DescImageUseShapeMask");
	obs_property_t *mask_shape = obs_properties_add_list(
		props, SETTING_IMAGE_MASK_SHAPE,
		obs_module_text("ImageMaskShape"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	add_shape_list(mask_shape);
	set_desc(props, SETTING_IMAGE_MASK_SHAPE, "DescImageMaskShape");

	/* ============================================================
	 * Shape Style group
	 * ============================================================ */
	obs_properties_t *style = obs_properties_create();
	obs_properties_add_color_alpha(style, SETTING_BASE_COLOR,
				       obs_module_text("ShapeColour"));
	set_desc(style, SETTING_BASE_COLOR, "DescShapeColour");
	obs_properties_add_color_alpha(style, SETTING_SECONDARY_COLOR,
				       obs_module_text("SecondaryColour"));
	set_desc(style, SETTING_SECONDARY_COLOR, "DescSecondaryColour");
	obs_property_t *color_mode = obs_properties_add_list(
		style, SETTING_COLOR_MODE, obs_module_text("SecondaryMode"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	set_desc(style, SETTING_COLOR_MODE, "DescSecondaryMode");
	obs_property_list_add_int(color_mode,
				  obs_module_text("ColorModeSolid"), 0);
	obs_property_list_add_int(color_mode,
				  obs_module_text("ColorModeRadial"), 1);
	obs_property_list_add_int(color_mode,
				  obs_module_text("ColorModeAlternate"), 2);
	obs_property_list_add_int(color_mode,
				  obs_module_text("ColorModeByShape"), 3);
	obs_properties_add_float_slider(style, SETTING_COLOR_MIX,
					obs_module_text("SecondaryMix"),
					0.0, 1.0, 0.01);
	set_desc(style, SETTING_COLOR_MIX, "DescSecondaryMix");
	obs_property_t *palette_count = obs_properties_add_list(
		style, SETTING_PALETTE_COUNT,
		obs_module_text("PaletteCount"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(palette_count, "1 (use base colour)", 1);
	obs_property_list_add_int(palette_count, "2 colours", 2);
	obs_property_list_add_int(palette_count, "3 colours", 3);
	obs_property_list_add_int(palette_count, "4 colours", 4);
	set_desc(style, SETTING_PALETTE_COUNT, "DescPaletteCount");
	obs_properties_add_color_alpha(style, SETTING_COLOR_B,
				       obs_module_text("ColorB"));
	set_desc(style, SETTING_COLOR_B, "DescColorB");
	obs_properties_add_color_alpha(style, SETTING_COLOR_C,
				       obs_module_text("ColorC"));
	set_desc(style, SETTING_COLOR_C, "DescColorC");
	obs_properties_add_color_alpha(style, SETTING_COLOR_D,
				       obs_module_text("ColorD"));
	set_desc(style, SETTING_COLOR_D, "DescColorD");
	obs_property_t *grid_pattern = obs_properties_add_list(
		style, SETTING_GRID_PATTERN, obs_module_text("GridPattern"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	set_desc(style, SETTING_GRID_PATTERN, "DescGridPattern");
	obs_property_list_add_int(grid_pattern,
				  obs_module_text("GridSquare"), 0);
	obs_property_list_add_int(grid_pattern,
				  obs_module_text("GridBrick"), 1);
	obs_property_list_add_int(grid_pattern,
				  obs_module_text("GridHex"), 2);
	obs_properties_add_bool(style, SETTING_SQUARE_CELLS,
				obs_module_text("SquareCells"));
	set_desc(style, SETTING_SQUARE_CELLS, "DescGridSquare");
	obs_properties_add_float_slider(style, SETTING_DENSITY,
					obs_module_text("Density"), 2.0,
					200.0, 1.0);
	set_desc(style, SETTING_DENSITY, "DescDensity");
	obs_properties_add_float_slider(style, SETTING_SHAPE_SIZE,
					obs_module_text("Size"), 0.0, 1.0,
					0.01);
	set_desc(style, SETTING_SHAPE_SIZE, "DescSize");
	obs_properties_add_float_slider(style, SETTING_GAP,
					obs_module_text("Gap"), 0.0, 0.95,
					0.01);
	set_desc(style, SETTING_GAP, "DescGap");
	obs_properties_add_float_slider(style, SETTING_PATTERN_SCALE,
					obs_module_text("PatternScale"),
					0.25, 4.0, 0.05);
	set_desc(style, SETTING_PATTERN_SCALE, "DescPatternScale");
	obs_properties_add_float_slider(style, SETTING_PATTERN_OFFSET_X,
					obs_module_text("PatternOffsetX"),
					-5.0, 5.0, 0.01);
	set_desc(style, SETTING_PATTERN_OFFSET_X, "DescPatternOffsetX");
	obs_properties_add_float_slider(style, SETTING_PATTERN_OFFSET_Y,
					obs_module_text("PatternOffsetY"),
					-5.0, 5.0, 0.01);
	set_desc(style, SETTING_PATTERN_OFFSET_Y, "DescPatternOffsetY");
	obs_properties_add_float_slider(style, SETTING_EDGE_SOFTNESS,
					obs_module_text("EdgeSoftness"),
					0.0, 0.2, 0.001);
	set_desc(style, SETTING_EDGE_SOFTNESS, "DescEdgeSoftness");
	obs_properties_add_group(props, "ShapeStyle",
				obs_module_text("GroupShapeStyle"),
				OBS_GROUP_NORMAL, style);

	/* ============================================================
	 * Shape Geometry group
	 * ============================================================ */
	obs_properties_t *geom = obs_properties_create();
	obs_properties_add_float_slider(geom, SETTING_ROTATION_DEG,
					obs_module_text("Rotation"), -180.0,
					180.0, 1.0);
	set_desc(geom, SETTING_ROTATION_DEG, "DescRotation");
	obs_properties_add_float_slider(geom, SETTING_AUTO_ROTATE_SPEED,
					obs_module_text("AutoRotate"), -180.0,
					180.0, 1.0);
	set_desc(geom, SETTING_AUTO_ROTATE_SPEED, "DescAutoRotate");
	obs_properties_add_int_slider(geom, SETTING_SHAPE_SIDES,
				      obs_module_text("PolygonSides"), 3, 12,
				      1);
	set_desc(geom, SETTING_SHAPE_SIDES, "DescPolygonSides");
	obs_properties_add_float_slider(geom, SETTING_STAR_SHARPNESS,
					obs_module_text("StarSharpness"),
					2.0, 12.0, 0.1);
	set_desc(geom, SETTING_STAR_SHARPNESS, "DescStarSharpness");
	obs_properties_add_float_slider(geom, SETTING_LINE_THICKNESS,
					obs_module_text("LineThickness"),
					0.01, 0.5, 0.01);
	set_desc(geom, SETTING_LINE_THICKNESS, "DescLineThickness");
	obs_properties_add_float_slider(geom, SETTING_ROUNDED_SQUARE_RADIUS,
					obs_module_text("RoundedSquareRadius"),
					0.0, 0.5, 0.01);
	set_desc(geom, SETTING_ROUNDED_SQUARE_RADIUS,
		 "DescRoundedSquareRadius");
	obs_properties_add_float_slider(geom, SETTING_STROKE_WIDTH,
					obs_module_text("StrokeWidth"),
					0.0, 0.2, 0.001);
	set_desc(geom, SETTING_STROKE_WIDTH, "DescStrokeWidth");
	obs_properties_add_color_alpha(geom, SETTING_STROKE_COLOUR,
				       obs_module_text("StrokeColour"));
	set_desc(geom, SETTING_STROKE_COLOUR, "DescStrokeColour");
	obs_properties_add_group(props, "ShapeGeometry",
				obs_module_text("GroupShapeGeometry"),
				OBS_GROUP_NORMAL, geom);

	/* ============================================================
	 * Variation group
	 * ============================================================ */
	obs_properties_t *var = obs_properties_create();
	obs_properties_add_float_slider(var, SETTING_VISIBILITY,
					obs_module_text("Visibility"), 0.0,
					1.0, 0.01);
	set_desc(var, SETTING_VISIBILITY, "DescVisibility");
	obs_properties_add_float_slider(var, SETTING_TWINKLE_AMOUNT,
					obs_module_text("TwinkleAmount"),
					0.0, 1.0, 0.01);
	set_desc(var, SETTING_TWINKLE_AMOUNT, "DescTwinkleAmount");
	obs_properties_add_float_slider(var, SETTING_TWINKLE_SPEED,
					obs_module_text("TwinkleSpeed"),
					0.0, 10.0, 0.1);
	set_desc(var, SETTING_TWINKLE_SPEED, "DescTwinkleSpeed");
	obs_properties_add_float_slider(var, SETTING_SIZE_VARIATION,
					obs_module_text("SizeVariation"),
					0.0, 1.0, 0.01);
	set_desc(var, SETTING_SIZE_VARIATION, "DescSizeVariation");
	obs_properties_add_float_slider(var, SETTING_ROTATION_VARIATION,
					obs_module_text("RotationVariation"),
					0.0, 180.0, 1.0);
	set_desc(var, SETTING_ROTATION_VARIATION, "DescRotationVariation");
	obs_properties_add_float_slider(var, SETTING_SHAPE_MIX_CHANCE,
					obs_module_text("ShapeMix"), 0.0,
					1.0, 0.01);
	set_desc(var, SETTING_SHAPE_MIX_CHANCE, "DescShapeMix");
	obs_properties_add_float_slider(
		var, SETTING_SHAPE_MIX_BLEND_WIDTH,
		obs_module_text("ShapeMixBlendWidth"), 0.0, 1.0, 0.01);
	set_desc(var, SETTING_SHAPE_MIX_BLEND_WIDTH,
		 "DescShapeMixBlendWidth");
	obs_properties_add_bool(
		var, SETTING_SHAPE_MIX_INDEPENDENT_MOTION,
		obs_module_text("ShapeMixIndependentMotion"));
	set_desc(var, SETTING_SHAPE_MIX_INDEPENDENT_MOTION,
		 "DescShapeMixIndependentMotion");
	obs_properties_add_float_slider(var, SETTING_HUE_VARIATION,
					obs_module_text("HueVariation"),
					0.0, 1.0, 0.01);
	set_desc(var, SETTING_HUE_VARIATION, "DescHueVariation");
	obs_properties_add_float_slider(var, SETTING_COLOR_JITTER,
					obs_module_text("ColorJitter"),
					0.0, 1.0, 0.01);
	set_desc(var, SETTING_COLOR_JITTER, "DescColorJitter");
	obs_properties_add_float_slider(var, SETTING_SEED,
					obs_module_text("Seed"), 0.0, 100.0,
					1.0);
	set_desc(var, SETTING_SEED, "DescSeed");
	obs_properties_add_button(var, "random_seed", obs_module_text("RandomSeed"),
				random_seed_callback);
	obs_property_t *gradient_axis = obs_properties_add_list(
		var, SETTING_GRADIENT_AXIS,
		obs_module_text("GradientAxis"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(gradient_axis,
				  obs_module_text("GradientNone"), 0);
	obs_property_list_add_int(gradient_axis,
				  obs_module_text("GradientHorizontal"), 1);
	obs_property_list_add_int(gradient_axis,
				  obs_module_text("GradientVertical"), 2);
	set_desc(var, SETTING_GRADIENT_AXIS, "DescGradientAxis");
	obs_properties_add_float_slider(
		var, SETTING_GRADIENT_SIZE_AMOUNT,
		obs_module_text("GradientSizeAmount"), -1.0, 1.0, 0.01);
	set_desc(var, SETTING_GRADIENT_SIZE_AMOUNT,
		 "DescGradientSizeAmount");
	obs_properties_add_float_slider(
		var, SETTING_GRADIENT_HUE_AMOUNT,
		obs_module_text("GradientHueAmount"), 0.0, 1.0, 0.01);
	set_desc(var, SETTING_GRADIENT_HUE_AMOUNT,
		 "DescGradientHueAmount");
	obs_properties_add_group(props, "Variation",
				obs_module_text("GroupVariation"),
				OBS_GROUP_NORMAL, var);

	/* ============================================================
	 * Motion group
	 * ============================================================ */
	obs_properties_t *mot = obs_properties_create();
	obs_properties_add_float_slider(mot, SETTING_SCROLL_SPEED,
					obs_module_text("ScrollSpeed"), 0.0,
					5.0, 0.01);
	set_desc(mot, SETTING_SCROLL_SPEED, "DescScrollSpeed");
	obs_properties_add_float_slider(mot, SETTING_SCROLL_ANGLE,
					obs_module_text("ScrollAngle"),
					0.0, 360.0, 1.0);
	set_desc(mot, SETTING_SCROLL_ANGLE, "DescScrollAngle");
	obs_properties_add_float_slider(mot, SETTING_DRIFT_AMOUNT,
					obs_module_text("DriftAmount"),
					0.0, 1.0, 0.01);
	set_desc(mot, SETTING_DRIFT_AMOUNT, "DescDriftAmount");
	obs_properties_add_float_slider(mot, SETTING_DRIFT_SPEED,
					obs_module_text("DriftSpeed"),
					0.0, 10.0, 0.1);
	set_desc(mot, SETTING_DRIFT_SPEED, "DescDriftSpeed");
	obs_properties_add_float_slider(mot, SETTING_PULSE_AMOUNT,
					obs_module_text("PulseAmount"),
					0.0, 1.0, 0.01);
	set_desc(mot, SETTING_PULSE_AMOUNT, "DescPulseAmount");
	obs_properties_add_float_slider(mot, SETTING_PULSE_SPEED,
					obs_module_text("PulseSpeed"),
					0.0, 10.0, 0.1);
	set_desc(mot, SETTING_PULSE_SPEED, "DescPulseSpeed");
	obs_properties_add_float_slider(mot, SETTING_WAVE_AMOUNT,
					obs_module_text("WaveAmount"),
					0.0, 1.0, 0.01);
	set_desc(mot, SETTING_WAVE_AMOUNT, "DescWaveAmount");
	obs_properties_add_float_slider(mot, SETTING_WAVE_FREQUENCY,
					obs_module_text("WaveFrequency"),
					0.1, 20.0, 0.1);
	set_desc(mot, SETTING_WAVE_FREQUENCY, "DescWaveFrequency");
	obs_properties_add_float_slider(mot, SETTING_WAVE_SPEED,
					obs_module_text("WaveSpeed"),
					0.0, 10.0, 0.1);
	set_desc(mot, SETTING_WAVE_SPEED, "DescWaveSpeed");
	obs_properties_add_float_slider(mot, SETTING_ORBIT_AMOUNT,
					obs_module_text("OrbitAmount"),
					-1.0, 1.0, 0.01);
	set_desc(mot, SETTING_ORBIT_AMOUNT, "DescOrbitAmount");
	obs_properties_add_float_slider(mot, SETTING_ORBIT_SPEED,
					obs_module_text("OrbitSpeed"),
					-180.0, 180.0, 1.0);
	set_desc(mot, SETTING_ORBIT_SPEED, "DescOrbitSpeed");
	obs_properties_add_group(props, "Motion",
				obs_module_text("GroupMotion"), OBS_GROUP_NORMAL,
				mot);

	obs_properties_add_button(props, "reset_defaults",
				obs_module_text("ResetToDefaults"),
				reset_to_defaults_callback);

	return props;
}

static void tilegen_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, SETTING_GRID_PATTERN, 0);
	obs_data_set_default_double(settings, SETTING_DENSITY, 20.0);
	obs_data_set_default_bool(settings, SETTING_SQUARE_CELLS, false);
	obs_data_set_default_double(settings, SETTING_GAP, 0.0);
	obs_data_set_default_double(settings, SETTING_SEED, 0.0);
	obs_data_set_default_double(settings, SETTING_PATTERN_SCALE, 1.0);
	obs_data_set_default_double(settings, SETTING_PATTERN_OFFSET_X, 0.0);
	obs_data_set_default_double(settings, SETTING_PATTERN_OFFSET_Y, 0.0);

	obs_data_set_default_int(settings, SETTING_SHAPE_TYPE, 0);
	obs_data_set_default_double(settings, SETTING_SHAPE_SIZE, 0.5);
	obs_data_set_default_double(settings, SETTING_EDGE_SOFTNESS, 0.02);
	obs_data_set_default_double(settings, SETTING_ROTATION_DEG, 0.0);
	obs_data_set_default_double(settings, SETTING_AUTO_ROTATE_SPEED, 0.0);
	obs_data_set_default_int(settings, SETTING_SHAPE_SIDES, 5);
	obs_data_set_default_double(settings, SETTING_STAR_SHARPNESS, 3.0);
	obs_data_set_default_double(settings, SETTING_STROKE_WIDTH, 0.0);
	obs_data_set_default_int(settings, SETTING_STROKE_COLOUR, 0xFFFFFFFF);

	obs_data_set_default_double(settings, SETTING_LINE_THICKNESS, 0.05);
	obs_data_set_default_double(settings, SETTING_ROUNDED_SQUARE_RADIUS,
				    0.15);

	obs_data_set_default_int(settings, SETTING_SHAPE_TYPE_B, 6);
	obs_data_set_default_double(settings, SETTING_SHAPE_MIX_CHANCE, 0.0);
	obs_data_set_default_double(settings, SETTING_SHAPE_MIX_BLEND_WIDTH,
				    0.0);
	obs_data_set_default_bool(settings,
				  SETTING_SHAPE_MIX_INDEPENDENT_MOTION,
				  true);

	obs_data_set_default_int(settings, SETTING_USE_SOURCE, 0);
	obs_data_set_default_double(settings, SETTING_IMAGE_ASPECT, 0.0);
	obs_data_set_default_string(settings, SETTING_IMAGE_PATH, "");
	obs_data_set_default_string(settings, SETTING_SOURCE_NAME, "");

	obs_data_set_default_int(settings, SETTING_FONT_COLS, 16);
	obs_data_set_default_int(settings, SETTING_FONT_ROWS, 16);
	obs_data_set_default_int(settings, SETTING_FONT_INDEX, 0);
	obs_data_set_default_double(settings, SETTING_FONT_ASPECT, 0.0);

	obs_data_set_default_int(settings, SETTING_BASE_COLOR, 0xFFFFFFFF);
	obs_data_set_default_int(settings, SETTING_SECONDARY_COLOR, 0xFFFF0000);
	obs_data_set_default_int(settings, SETTING_COLOR_MODE, 0);
	obs_data_set_default_double(settings, SETTING_COLOR_MIX, 0.0);

	obs_data_set_default_int(settings, SETTING_PALETTE_COUNT, 1);
	obs_data_set_default_int(settings, SETTING_COLOR_B, 0xFFFF0000);
	obs_data_set_default_int(settings, SETTING_COLOR_C, 0xFF00FF00);
	obs_data_set_default_int(settings, SETTING_COLOR_D, 0xFF0000FF);

	obs_data_set_default_double(settings, SETTING_VISIBILITY, 1.0);
	obs_data_set_default_double(settings, SETTING_SIZE_VARIATION, 0.0);
	obs_data_set_default_double(settings, SETTING_ROTATION_VARIATION, 0.0);
	obs_data_set_default_double(settings, SETTING_HUE_VARIATION, 0.0);
	obs_data_set_default_double(settings, SETTING_COLOR_JITTER, 0.0);
	obs_data_set_default_double(settings, SETTING_TWINKLE_AMOUNT, 0.0);
	obs_data_set_default_double(settings, SETTING_TWINKLE_SPEED, 1.0);

	obs_data_set_default_double(settings, SETTING_SCROLL_SPEED, 0.0);
	obs_data_set_default_double(settings, SETTING_SCROLL_ANGLE, 0.0);
	obs_data_set_default_double(settings, SETTING_DRIFT_AMOUNT, 0.0);
	obs_data_set_default_double(settings, SETTING_DRIFT_SPEED, 1.0);
	obs_data_set_default_double(settings, SETTING_PULSE_AMOUNT, 0.0);
	obs_data_set_default_double(settings, SETTING_PULSE_SPEED, 1.0);

	obs_data_set_default_double(settings, SETTING_WAVE_AMOUNT, 0.0);
	obs_data_set_default_double(settings, SETTING_WAVE_FREQUENCY, 4.0);
	obs_data_set_default_double(settings, SETTING_WAVE_SPEED, 1.0);
	obs_data_set_default_double(settings, SETTING_ORBIT_AMOUNT, 0.0);
	obs_data_set_default_double(settings, SETTING_ORBIT_SPEED, 30.0);

	obs_data_set_default_int(settings, SETTING_GRADIENT_AXIS, 0);
	obs_data_set_default_double(settings, SETTING_GRADIENT_SIZE_AMOUNT,
				    0.0);
	obs_data_set_default_double(settings, SETTING_GRADIENT_HUE_AMOUNT,
				    0.0);

	obs_data_set_default_double(settings, SETTING_VIGNETTE_AMOUNT, 0.0);
	obs_data_set_default_double(settings, SETTING_VIGNETTE_SIZE, 0.5);
	obs_data_set_default_double(settings, SETTING_VIGNETTE_SOFTNESS, 0.5);

	obs_data_set_default_double(settings, SETTING_GLOW_AMOUNT, 0.0);
	obs_data_set_default_double(settings, SETTING_GLOW_RADIUS, 0.1);
	obs_data_set_default_int(settings, SETTING_GLOW_COLOUR, 0xFFFFFFFF);

	obs_data_set_default_int(settings, SETTING_USE_INTERNAL_BG, 1);
	obs_data_set_default_int(settings, SETTING_BACKGROUND_COLOR, 0xFF000000);

	// Presets
	obs_data_set_default_int(settings, SETTING_PREDEFINED_PRESET, -1);

	// Image transform
	obs_data_set_default_double(settings, SETTING_IMAGE_ROTATION, 0.0);
	obs_data_set_default_double(settings, SETTING_IMAGE_SCALE, 1.0);
	obs_data_set_default_double(settings, SETTING_IMAGE_OFFSET_X, 0.0);
	obs_data_set_default_double(settings, SETTING_IMAGE_OFFSET_Y, 0.0);

	obs_data_set_default_int(settings, SETTING_IMAGE_FIT_MODE, 0);
	obs_data_set_default_bool(settings, SETTING_IMAGE_USE_SHAPE_MASK,
				  false);
	obs_data_set_default_int(settings, SETTING_IMAGE_MASK_SHAPE, 0);

	// Performance
	obs_data_set_default_double(settings, SETTING_RENDER_SCALE, 1.0);
}

struct obs_source_info tilegen_filter = {
	.id = "tilegen_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = tilegen_name,
	.create = tilegen_create,
	.destroy = tilegen_destroy,
	.update = tilegen_update,
	.video_render = tilegen_video_render,
	.get_properties = tilegen_properties,
	.get_defaults = tilegen_defaults,
};
