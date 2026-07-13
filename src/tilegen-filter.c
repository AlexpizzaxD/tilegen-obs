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

struct tilegen_filter {
	obs_source_t *source;
	gs_effect_t *effect;
};

static const char *tilegen_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("TileGen");
}

static void tilegen_update(void *data, obs_data_t *settings)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(settings);
}

static void *tilegen_create(obs_data_t *settings, obs_source_t *source)
{
	struct tilegen_filter *tf = bzalloc(sizeof(struct tilegen_filter));
	tf->source = source;

	char *effect_path = obs_module_file("effects/tilegen.effect");
	if (effect_path) {
		tf->effect = gs_effect_create_from_file(effect_path, NULL);
		bfree(effect_path);
	}

	if (!tf->effect) {
		obs_log(LOG_ERROR, "Failed to load tilegen.effect");
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

static void tilegen_video_render(void *data, gs_effect_t *effect)
{
	struct tilegen_filter *tf = data;

	if (!obs_source_process_filter_begin(tf->source, GS_RGBA,
					     OBS_ALLOW_DIRECT_RENDERING)) {
		return;
	}

	if (tf->effect) {
		obs_source_process_filter_end(tf->source, tf->effect, 0, 0);
	} else {
		obs_source_process_filter_end(tf->source, effect, 0, 0);
	}
}

static obs_properties_t *tilegen_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();
	UNUSED_PARAMETER(data);
	return props;
}

static void tilegen_defaults(obs_data_t *settings)
{
	UNUSED_PARAMETER(settings);
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
