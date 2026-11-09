#pragma once

#include <obs-module.h>
#include <graphics/vec2.h>
#include <graphics/matrix4.h>
#include "text-scale-math.h"
#include "text-measurement.h"

struct text_scale_filter {
	obs_source_t *context = nullptr;

	gs_effect_t *effect = nullptr;
	gs_eparam_t *param_mul = nullptr;
	gs_eparam_t *param_add = nullptr;
	gs_eparam_t *param_image = nullptr;
	gs_eparam_t *param_multiplier = nullptr;

	textscale::ScaleConfig config;
	textscale::ScaleResult result;
	textscale::TextMeasurement measurement;

	uint32_t base_cx = 0;
	uint32_t base_cy = 0;
	double measured_content_width = 0.0;
	bool is_gdiplus_source = false;
	bool target_valid = false;

	struct vec2 mul_val;
	struct vec2 add_val;
};

extern struct obs_source_info text_scale_filter_info;
