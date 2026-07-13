/*
TileGen OBS
Copyright (C) 2026 LaViduka

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#include <obs-module.h>
#include <plugin-support.h>
#include "tilegen-filter.h"

#define SETTING_GRID_PATTERN "grid_pattern"
#define SETTING_DENSITY "density"
#define SETTING_SQUARE_CELLS "square_cells"
#define SETTING_GAP "gap"

#define SETTING_SHAPE_TYPE "shape_type"
#define SETTING_SHAPE_SIZE "shape_size"
#define SETTING_EDGE_SOFTNESS "edge_softness"
#define SETTING_ROTATION_DEG "rotation_deg"
#define SETTING_AUTO_ROTATE_SPEED "auto_rotate_speed"
#define SETTING_SHAPE_SIDES "shape_sides"
#define SETTING_STAR_SHARPNESS "star_sharpness"

#define SETTING_SHAPE_TYPE_B "shape_type_b"
#define SETTING_SHAPE_MIX_CHANCE "shape_mix_chance"

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

#define SETTING_VISIBILITY "visibility_chance"
#define SETTING_SIZE_VARIATION "size_variation"
#define SETTING_ROTATION_VARIATION "rotation_variation"
#define SETTING_HUE_VARIATION "hue_variation"
#define SETTING_TWINKLE_AMOUNT "twinkle_amount"
#define SETTING_TWINKLE_SPEED "twinkle_speed"

#define SETTING_SCROLL_SPEED "scroll_speed"
#define SETTING_SCROLL_OFFSET "scroll_offset"
#define SETTING_DRIFT_AMOUNT "drift_amount"
#define SETTING_DRIFT_SPEED "drift_speed"
#define SETTING_PULSE_AMOUNT "pulse_amount"
#define SETTING_PULSE_SPEED "pulse_speed"

struct tilegen_filter {
	obs_source_t *source;
	gs_effect_t *effect;
	struct tilegen_settings settings;
	uint64_t start_time;
};

static const char *tilegen_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("TileGen");
}

static void tilegen_update(void *data, obs_data_t *settings)
{
	struct tilegen_filter *tf = data;

	tf->settings.grid_pattern =
		(int)obs_data_get_int(settings, SETTING_GRID_PATTERN);
	tf->settings.density =
		(float)obs_data_get_double(settings, SETTING_DENSITY);
	tf->settings.square_cells =
		obs_data_get_bool(settings, SETTING_SQUARE_CELLS);
	tf->settings.gap = (float)obs_data_get_double(settings, SETTING_GAP);

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

	tf->settings.shape_type_b =
		(int)obs_data_get_int(settings, SETTING_SHAPE_TYPE_B);
	tf->settings.shape_mix_chance =
		(float)obs_data_get_double(settings, SETTING_SHAPE_MIX_CHANCE);

	tf->settings.use_source_instead_of_image =
		(int)obs_data_get_int(settings, SETTING_USE_SOURCE);
	tf->settings.image_aspect_override =
		(float)obs_data_get_double(settings, SETTING_IMAGE_ASPECT);

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

	tf->settings.visibility_chance =
		(float)obs_data_get_double(settings, SETTING_VISIBILITY);
	tf->settings.size_variation =
		(float)obs_data_get_double(settings, SETTING_SIZE_VARIATION);
	tf->settings.rotation_variation =
		(float)obs_data_get_double(settings, SETTING_ROTATION_VARIATION);
	tf->settings.hue_variation =
		(float)obs_data_get_double(settings, SETTING_HUE_VARIATION);
	tf->settings.twinkle_amount =
		(float)obs_data_get_double(settings, SETTING_TWINKLE_AMOUNT);
	tf->settings.twinkle_speed =
		(float)obs_data_get_double(settings, SETTING_TWINKLE_SPEED);

	obs_data_get_vec2(settings, SETTING_SCROLL_SPEED,
			  &tf->settings.scroll_speed);
	obs_data_get_vec2(settings, SETTING_SCROLL_OFFSET,
			  &tf->settings.scroll_offset);

	tf->settings.drift_amount =
		(float)obs_data_get_double(settings, SETTING_DRIFT_AMOUNT);
	tf->settings.drift_speed =
		(float)obs_data_get_double(settings, SETTING_DRIFT_SPEED);
	tf->settings.pulse_amount =
		(float)obs_data_get_double(settings, SETTING_PULSE_AMOUNT);
	tf->settings.pulse_speed =
		(float)obs_data_get_double(settings, SETTING_PULSE_SPEED);
}

static void *tilegen_create(obs_data_t *settings, obs_source_t *source)
{
	struct tilegen_filter *tf = bzalloc(sizeof(struct tilegen_filter));
	tf->source = source;
	tf->start_time = obs_get_video_frame_time();

	char *effect_path = obs_module_file("effects/tilegen.effect");
	if (effect_path) {
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
		bfree(effect_path);
	}

	tilegen_update(tf, settings);
	return tf;
}

static void tilegen_destroy(void *data)
{
	struct tilegen_filter *tf = data;
	if (tf->effect)
		gs_effect_destroy(tf->effect);
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

		set_uniform_int(tf->effect, "shape_type_b",
				tf->settings.shape_type_b);
		set_uniform_float(tf->effect, "shape_mix_chance",
				  tf->settings.shape_mix_chance);

		set_uniform_int(tf->effect, "use_source_instead_of_image",
				tf->settings.use_source_instead_of_image);
		set_uniform_float(tf->effect, "image_aspect_override",
				  tf->settings.image_aspect_override);

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

		set_uniform_float(tf->effect, "visibility_chance",
				  tf->settings.visibility_chance);
		set_uniform_float(tf->effect, "size_variation",
				  tf->settings.size_variation);
		set_uniform_float(tf->effect, "rotation_variation",
				  tf->settings.rotation_variation);
		set_uniform_float(tf->effect, "hue_variation",
				  tf->settings.hue_variation);
		set_uniform_float(tf->effect, "twinkle_amount",
				  tf->settings.twinkle_amount);
		set_uniform_float(tf->effect, "twinkle_speed",
				  tf->settings.twinkle_speed);

		set_uniform_vec2(tf->effect, "scroll_speed",
				 &tf->settings.scroll_speed);
		set_uniform_vec2(tf->effect, "scroll_offset",
				 &tf->settings.scroll_offset);
		set_uniform_float(tf->effect, "drift_amount",
				  tf->settings.drift_amount);
		set_uniform_float(tf->effect, "drift_speed",
				  tf->settings.drift_speed);
		set_uniform_float(tf->effect, "pulse_amount",
				  tf->settings.pulse_amount);
		set_uniform_float(tf->effect, "pulse_speed",
				  tf->settings.pulse_speed);

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
	obs_property_list_add_int(list, "Character (Atlas)", 8);
}

static obs_properties_t *tilegen_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();
	UNUSED_PARAMETER(data);

	obs_property_t *grid_pattern = obs_properties_add_list(
		props, SETTING_GRID_PATTERN,
		obs_module_text("GridPattern"), OBS_COMBO_TYPE_LIST,
		OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(grid_pattern,
				  obs_module_text("GridSquare"), 0);
	obs_property_list_add_int(grid_pattern,
				  obs_module_text("GridBrick"), 1);
	obs_property_list_add_int(grid_pattern,
				  obs_module_text("GridHex"), 2);

	obs_properties_add_float_slider(props, SETTING_DENSITY,
					obs_module_text("Density"),
					2.0, 200.0, 1.0);
	obs_properties_add_bool(props, SETTING_SQUARE_CELLS,
				obs_module_text("SquareCells"));
	obs_properties_add_float_slider(props, SETTING_GAP,
					obs_module_text("Gap"), 0.0,
					0.95, 0.01);

	obs_property_t *shape_type = obs_properties_add_list(
		props, SETTING_SHAPE_TYPE, obs_module_text("ShapeType"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	add_shape_list(shape_type);

	obs_properties_add_float_slider(props, SETTING_SHAPE_SIZE,
					obs_module_text("ShapeSize"),
					0.0, 1.0, 0.01);
	obs_properties_add_float_slider(props, SETTING_EDGE_SOFTNESS,
					obs_module_text("EdgeSoftness"),
					0.0, 0.2, 0.001);
	obs_properties_add_float_slider(props, SETTING_ROTATION_DEG,
					obs_module_text("Rotation"),
					-180.0, 180.0, 1.0);
	obs_properties_add_float_slider(props, SETTING_AUTO_ROTATE_SPEED,
					obs_module_text("AutoRotateSpeed"),
					-180.0, 180.0, 1.0);
	obs_properties_add_int_slider(props, SETTING_SHAPE_SIDES,
				      obs_module_text("ShapeSides"), 3,
				      12, 1);
	obs_properties_add_float_slider(props, SETTING_STAR_SHARPNESS,
					obs_module_text("StarSharpness"),
					2.0, 12.0, 0.1);

	obs_properties_add_color_alpha(props, SETTING_BASE_COLOR,
				       obs_module_text("BaseColor"));

	return props;
}

static void tilegen_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, SETTING_GRID_PATTERN, 0);
	obs_data_set_default_double(settings, SETTING_DENSITY, 20.0);
	obs_data_set_default_bool(settings, SETTING_SQUARE_CELLS, false);
	obs_data_set_default_double(settings, SETTING_GAP, 0.0);

	obs_data_set_default_int(settings, SETTING_SHAPE_TYPE, 0);
	obs_data_set_default_double(settings, SETTING_SHAPE_SIZE, 0.5);
	obs_data_set_default_double(settings, SETTING_EDGE_SOFTNESS, 0.02);
	obs_data_set_default_double(settings, SETTING_ROTATION_DEG, 0.0);
	obs_data_set_default_double(settings, SETTING_AUTO_ROTATE_SPEED, 0.0);
	obs_data_set_default_int(settings, SETTING_SHAPE_SIDES, 5);
	obs_data_set_default_double(settings, SETTING_STAR_SHARPNESS, 3.0);

	obs_data_set_default_int(settings, SETTING_SHAPE_TYPE_B, 6);
	obs_data_set_default_double(settings, SETTING_SHAPE_MIX_CHANCE, 0.0);

	obs_data_set_default_int(settings, SETTING_USE_SOURCE, 1);
	obs_data_set_default_double(settings, SETTING_IMAGE_ASPECT, 0.0);

	obs_data_set_default_int(settings, SETTING_FONT_COLS, 16);
	obs_data_set_default_int(settings, SETTING_FONT_ROWS, 16);
	obs_data_set_default_int(settings, SETTING_FONT_INDEX, 0);
	obs_data_set_default_double(settings, SETTING_FONT_ASPECT, 0.0);

	obs_data_set_default_int(settings, SETTING_BASE_COLOR, 0xFFFFFFFF);
	obs_data_set_default_int(settings, SETTING_SECONDARY_COLOR, 0xFFFF0000);
	obs_data_set_default_int(settings, SETTING_COLOR_MODE, 0);
	obs_data_set_default_double(settings, SETTING_COLOR_MIX, 0.0);

	obs_data_set_default_double(settings, SETTING_VISIBILITY, 1.0);
	obs_data_set_default_double(settings, SETTING_SIZE_VARIATION, 0.0);
	obs_data_set_default_double(settings, SETTING_ROTATION_VARIATION, 0.0);
	obs_data_set_default_double(settings, SETTING_HUE_VARIATION, 0.0);
	obs_data_set_default_double(settings, SETTING_TWINKLE_AMOUNT, 0.0);
	obs_data_set_default_double(settings, SETTING_TWINKLE_SPEED, 1.0);

	obs_data_set_default_double(settings, "scroll_speed.x", 0.0);
	obs_data_set_default_double(settings, "scroll_speed.y", 0.0);
	obs_data_set_default_double(settings, "scroll_offset.x", 0.0);
	obs_data_set_default_double(settings, "scroll_offset.y", 0.0);
	obs_data_set_default_double(settings, SETTING_DRIFT_AMOUNT, 0.0);
	obs_data_set_default_double(settings, SETTING_DRIFT_SPEED, 1.0);
	obs_data_set_default_double(settings, SETTING_PULSE_AMOUNT, 0.0);
	obs_data_set_default_double(settings, SETTING_PULSE_SPEED, 1.0);
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
