#include "text-scale-filter.h"
#include <util/dstr.h>
#include <util/platform.h>

#define S_ENABLE_MANUAL      "enable_manual"
#define S_SCALE_X            "scale_x"
#define S_SCALE_Y            "scale_y"
#define S_RESET_MANUAL       "reset_manual"

#define S_ENABLE_AUTO        "enable_auto"
#define S_TARGET_WIDTH       "target_width"
#define S_MIN_SCALE          "min_scale"
#define S_MAX_SCALE          "max_scale"
#define S_PRESERVE_HEIGHT    "preserve_height"

#define S_ALIGNMENT          "alignment"

#define S_HIGH_QUALITY       "high_quality"
#define S_DEBUG_INFO         "debug_info"
#define S_RESET_DEFAULTS     "reset_defaults"

#define T_(text)             obs_module_text(text)

static const char *text_scale_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "TextScale";
}

static void *text_scale_create(obs_data_t *settings, obs_source_t *context)
{
	struct text_scale_filter *filter = new text_scale_filter();
	filter->context = context;

	char *effect_path = obs_module_file("text_scale.effect");
	if (!effect_path || !os_file_exists(effect_path)) {
		bfree(effect_path);
		effect_path = nullptr;

		const char *appdata = getenv("APPDATA");
		if (appdata) {
			char appdata_path[512] = {0};
			snprintf(appdata_path, sizeof(appdata_path),
				 "%s/obs-studio/plugins/text-scale/data/text_scale.effect", appdata);
			if (os_file_exists(appdata_path)) {
				effect_path = bstrdup(appdata_path);
			}
		}

		if (!effect_path) {
			const char *programdata = "C:/ProgramData/obs-studio/plugins/text-scale/data/text_scale.effect";
			if (os_file_exists(programdata)) {
				effect_path = bstrdup(programdata);
			}
		}
	}

	char *error_string = nullptr;
	obs_enter_graphics();
	filter->effect = gs_effect_create_from_file(effect_path, &error_string);
	obs_leave_graphics();

	if (!filter->effect) {
		blog(LOG_ERROR, "[TextScale] Failed to load text_scale.effect from '%s': %s",
		     effect_path ? effect_path : "<null>",
		     error_string ? error_string : "unknown error");
		bfree(error_string);
		bfree(effect_path);
		delete filter;
		return nullptr;
	}

	bfree(error_string);
	bfree(effect_path);

	filter->param_mul = gs_effect_get_param_by_name(filter->effect, "mul_val");
	filter->param_add = gs_effect_get_param_by_name(filter->effect, "add_val");
	filter->param_image = gs_effect_get_param_by_name(filter->effect, "image");
	filter->param_multiplier = gs_effect_get_param_by_name(filter->effect, "multiplier");

	obs_source_update(context, settings);
	return filter;
}

static void text_scale_destroy(void *data)
{
	struct text_scale_filter *filter = static_cast<struct text_scale_filter *>(data);
	if (!filter)
		return;

	obs_enter_graphics();
	if (filter->effect) {
		gs_effect_destroy(filter->effect);
		filter->effect = nullptr;
	}
	obs_leave_graphics();

	delete filter;
}

static void text_scale_update(void *data, obs_data_t *settings)
{
	struct text_scale_filter *filter = static_cast<struct text_scale_filter *>(data);
	if (!filter)
		return;

	filter->config.enable_manual_scale = obs_data_get_bool(settings, S_ENABLE_MANUAL);
	filter->config.manual_scale_x = (double)obs_data_get_int(settings, S_SCALE_X) / 100.0;
	filter->config.manual_scale_y = (double)obs_data_get_int(settings, S_SCALE_Y) / 100.0;

	filter->config.enable_auto_condense = obs_data_get_bool(settings, S_ENABLE_AUTO);
	filter->config.target_width = (double)obs_data_get_int(settings, S_TARGET_WIDTH);
	filter->config.min_scale = (double)obs_data_get_int(settings, S_MIN_SCALE) / 100.0;
	filter->config.max_scale = (double)obs_data_get_int(settings, S_MAX_SCALE) / 100.0;
	filter->config.preserve_height = obs_data_get_bool(settings, S_PRESERVE_HEIGHT);

	int align_val = (int)obs_data_get_int(settings, S_ALIGNMENT);
	if (align_val == 1)
		filter->config.alignment = textscale::Alignment::Center;
	else if (align_val == 2)
		filter->config.alignment = textscale::Alignment::Right;
	else
		filter->config.alignment = textscale::Alignment::Left;

	filter->config.high_quality = obs_data_get_bool(settings, S_HIGH_QUALITY);
	filter->config.debug_info = obs_data_get_bool(settings, S_DEBUG_INFO);

	filter->measurement.InvalidateCache();
}

static void text_scale_video_tick(void *data, float seconds)
{
	UNUSED_PARAMETER(seconds);
	struct text_scale_filter *filter = static_cast<struct text_scale_filter *>(data);
	if (!filter || !filter->context)
		return;

	obs_source_t *target = obs_filter_get_target(filter->context);
	if (!target) {
		filter->target_valid = false;
		return;
	}

	filter->base_cx = obs_source_get_base_width(target);
	filter->base_cy = obs_source_get_base_height(target);

	if (filter->base_cx == 0 || filter->base_cy == 0) {
		filter->target_valid = false;
		return;
	}

	filter->target_valid = true;

	// Measure visible content width
	filter->measurement.MeasureTargetContentWidth(target, filter->measured_content_width, filter->is_gdiplus_source);

	// Compute scaling transform
	filter->result = textscale::CalculateScaleTransform(filter->base_cx, filter->base_cy,
							    filter->measured_content_width, filter->config);

	// Calculate UV transformation
	double scale_x = filter->result.scale_x;
	double scale_y = filter->result.scale_y;
	double offset_x = filter->result.render_offset_x;
	double offset_y = filter->result.render_offset_y;

	double denom_x = (double)filter->base_cx * scale_x;
	double denom_y = (double)filter->base_cy * scale_y;

	if (denom_x > 0.0) {
		filter->mul_val.x = (float)((double)filter->result.output_width / denom_x);
		filter->add_val.x = (float)(-offset_x / denom_x);
	} else {
		filter->mul_val.x = 1.0f;
		filter->add_val.x = 0.0f;
	}

	if (denom_y > 0.0) {
		filter->mul_val.y = (float)((double)filter->result.output_height / denom_y);
		filter->add_val.y = (float)(-offset_y / denom_y);
	} else {
		filter->mul_val.y = 1.0f;
		filter->add_val.y = 0.0f;
	}

	if (filter->config.debug_info) {
		static float debug_timer = 0.0f;
		debug_timer += seconds;
		if (debug_timer >= 2.0f) {
			debug_timer = 0.0f;
			blog(LOG_INFO,
			     "[TextScale Debug] Base: %ux%u | ContentW: %.1f | Auto: %d | ScaleX: %.1f%% | Out: %ux%u | OffsetX: %.1f",
			     filter->base_cx, filter->base_cy, filter->measured_content_width,
			     filter->config.enable_auto_condense ? 1 : 0, filter->result.scale_x * 100.0,
			     filter->result.output_width, filter->result.output_height,
			     filter->result.render_offset_x);
		}
	}
}

static const char *get_technique_name_and_multiplier(enum gs_color_space current_space,
						     enum gs_color_space source_space,
						     float *multiplier)
{
	const char *tech_name = "Draw";
	*multiplier = 1.0f;

	switch (source_space) {
	case GS_CS_SRGB:
	case GS_CS_SRGB_16F:
		if (current_space == GS_CS_709_SCRGB) {
			tech_name = "DrawMultiply";
			*multiplier = obs_get_video_sdr_white_level() / 80.0f;
		}
		break;
	case GS_CS_709_EXTENDED:
		switch (current_space) {
		case GS_CS_SRGB:
		case GS_CS_SRGB_16F:
			tech_name = "DrawTonemap";
			break;
		case GS_CS_709_SCRGB:
			tech_name = "DrawMultiply";
			*multiplier = obs_get_video_sdr_white_level() / 80.0f;
			break;
		case GS_CS_709_EXTENDED:
			break;
		}
		break;
	case GS_CS_709_SCRGB:
		switch (current_space) {
		case GS_CS_SRGB:
		case GS_CS_SRGB_16F:
			tech_name = "DrawMultiplyTonemap";
			*multiplier = 80.0f / obs_get_video_sdr_white_level();
			break;
		case GS_CS_709_EXTENDED:
			tech_name = "DrawMultiply";
			*multiplier = 80.0f / obs_get_video_sdr_white_level();
			break;
		case GS_CS_709_SCRGB:
			break;
		}
	}

	return tech_name;
}

static void text_scale_video_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);
	struct text_scale_filter *filter = static_cast<struct text_scale_filter *>(data);
	if (!filter || !filter->context || !filter->target_valid || !filter->effect) {
		obs_source_skip_video_filter(filter ? filter->context : nullptr);
		return;
	}

	const enum gs_color_space preferred_spaces[] = {
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
	};

	obs_source_t *target = obs_filter_get_target(filter->context);
	const enum gs_color_space source_space = obs_source_get_color_space(
		target, OBS_COUNTOF(preferred_spaces), preferred_spaces);

	float multiplier = 1.0f;
	const char *technique = get_technique_name_and_multiplier(gs_get_color_space(), source_space, &multiplier);
	const enum gs_color_format format = gs_get_format_from_space(source_space);

	if (obs_source_process_filter_begin_with_color_space(filter->context, format, source_space,
							     OBS_NO_DIRECT_RENDERING)) {
		gs_effect_set_vec2(filter->param_mul, &filter->mul_val);
		gs_effect_set_vec2(filter->param_add, &filter->add_val);
		gs_effect_set_float(filter->param_multiplier, multiplier);

		gs_blend_state_push();
		gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

		obs_source_process_filter_tech_end(filter->context, filter->effect,
						   filter->result.output_width,
						   filter->result.output_height,
						   technique);

		gs_blend_state_pop();
	}
}

static uint32_t text_scale_get_width(void *data)
{
	struct text_scale_filter *filter = static_cast<struct text_scale_filter *>(data);
	if (!filter)
		return 0;
	return filter->result.output_width > 0 ? filter->result.output_width : filter->base_cx;
}

static uint32_t text_scale_get_height(void *data)
{
	struct text_scale_filter *filter = static_cast<struct text_scale_filter *>(data);
	if (!filter)
		return 0;
	return filter->result.output_height > 0 ? filter->result.output_height : filter->base_cy;
}

static enum gs_color_space text_scale_get_color_space(void *data, size_t count,
						      const enum gs_color_space *preferred_spaces)
{
	const enum gs_color_space potential_spaces[] = {
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
	};

	struct text_scale_filter *const filter = static_cast<struct text_scale_filter *>(data);
	obs_source_t *target = obs_filter_get_target(filter->context);
	const enum gs_color_space source_space = obs_source_get_color_space(
		target, OBS_COUNTOF(potential_spaces), potential_spaces);

	enum gs_color_space space = source_space;
	for (size_t i = 0; i < count; ++i) {
		space = preferred_spaces[i];
		if (space == source_space)
			break;
	}

	return space;
}

static bool reset_scaling_clicked(obs_properties_t *props, obs_property_t *p, void *data)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(p);
	struct text_scale_filter *filter = static_cast<struct text_scale_filter *>(data);
	if (filter && filter->context) {
		obs_data_t *settings = obs_source_get_settings(filter->context);
		obs_data_set_int(settings, S_SCALE_X, 100);
		obs_data_set_int(settings, S_SCALE_Y, 100);
		obs_source_update(filter->context, settings);
		obs_data_release(settings);
	}
	return true;
}

static bool reset_defaults_clicked(obs_properties_t *props, obs_property_t *p, void *data)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(p);
	struct text_scale_filter *filter = static_cast<struct text_scale_filter *>(data);
	if (filter && filter->context) {
		obs_data_t *settings = obs_source_get_settings(filter->context);
		obs_data_clear(settings);
		obs_source_update(filter->context, settings);
		obs_data_release(settings);
	}
	return true;
}

static void text_scale_get_defaults(obs_data_t *settings)
{
	// Group A – Scaling
	obs_data_set_default_bool(settings, S_ENABLE_MANUAL, false);
	obs_data_set_default_int(settings, S_SCALE_X, 100);
	obs_data_set_default_int(settings, S_SCALE_Y, 100);

	// Group B – Auto Condense
	obs_data_set_default_bool(settings, S_ENABLE_AUTO, false);
	obs_data_set_default_int(settings, S_TARGET_WIDTH, 267);
	obs_data_set_default_int(settings, S_MIN_SCALE, 65);
	obs_data_set_default_int(settings, S_MAX_SCALE, 100);
	obs_data_set_default_bool(settings, S_PRESERVE_HEIGHT, true);

	// Group C – Alignment
	obs_data_set_default_int(settings, S_ALIGNMENT, 0); // 0 = Left

	// Group D – Advanced
	obs_data_set_default_bool(settings, S_HIGH_QUALITY, true);
	obs_data_set_default_bool(settings, S_DEBUG_INFO, false);
}

static obs_properties_t *text_scale_get_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	// -------------------------------------------------------------
	// Group A: Scaling
	// -------------------------------------------------------------
	obs_properties_t *group_manual = obs_properties_create();
	obs_properties_add_bool(group_manual, S_ENABLE_MANUAL, "Enable Manual Scaling");
	obs_properties_add_int_slider(group_manual, S_SCALE_X, "Horizontal Scale (%)", 10, 100, 1);
	obs_properties_add_int_slider(group_manual, S_SCALE_Y, "Vertical Scale (%)", 10, 200, 1);
	obs_properties_add_button(group_manual, S_RESET_MANUAL, "Reset Scaling", reset_scaling_clicked);
	obs_properties_add_group(props, "group_manual", "Group A - Scaling", OBS_GROUP_NORMAL, group_manual);

	// -------------------------------------------------------------
	// Group B: Auto Condense (The Primary Feature)
	// -------------------------------------------------------------
	obs_properties_t *group_auto = obs_properties_create();
	obs_properties_add_bool(group_auto, S_ENABLE_AUTO, "Enable Auto Condense");
	obs_properties_add_int(group_auto, S_TARGET_WIDTH, "Target Width (px)", 10, 8192, 1);
	obs_properties_add_int_slider(group_auto, S_MIN_SCALE, "Minimum Scale (%)", 10, 100, 1);
	obs_properties_add_int_slider(group_auto, S_MAX_SCALE, "Maximum Scale (%)", 10, 100, 1);
	obs_properties_add_bool(group_auto, S_PRESERVE_HEIGHT, "Preserve Height");
	obs_properties_add_group(props, "group_auto", "Group B - Auto Condense", OBS_GROUP_NORMAL, group_auto);

	// -------------------------------------------------------------
	// Group C: Alignment
	// -------------------------------------------------------------
	obs_properties_t *group_align = obs_properties_create();
	obs_property_t *p_align = obs_properties_add_list(group_align, S_ALIGNMENT, "Alignment",
							  OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(p_align, "Left", 0);
	obs_property_list_add_int(p_align, "Center", 1);
	obs_property_list_add_int(p_align, "Right", 2);
	obs_properties_add_group(props, "group_align", "Group C - Alignment", OBS_GROUP_NORMAL, group_align);

	// -------------------------------------------------------------
	// Group D: Advanced
	// -------------------------------------------------------------
	obs_properties_t *group_adv = obs_properties_create();
	obs_properties_add_bool(group_adv, S_HIGH_QUALITY, "High Quality Scaling");
	obs_properties_add_bool(group_adv, S_DEBUG_INFO, "Debug Information");
	obs_properties_add_button(group_adv, S_RESET_DEFAULTS, "Reset to Defaults", reset_defaults_clicked);
	obs_properties_add_group(props, "group_adv", "Group D - Advanced", OBS_GROUP_NORMAL, group_adv);

	UNUSED_PARAMETER(data);
	return props;
}

static struct obs_source_info init_source_info()
{
	struct obs_source_info si = {};
	si.id = "text_scale_filter";
	si.type = OBS_SOURCE_TYPE_FILTER;
	si.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB;
	si.get_name = text_scale_get_name;
	si.create = text_scale_create;
	si.destroy = text_scale_destroy;
	si.get_width = text_scale_get_width;
	si.get_height = text_scale_get_height;
	si.get_defaults = text_scale_get_defaults;
	si.get_properties = text_scale_get_properties;
	si.update = text_scale_update;
	si.video_tick = text_scale_video_tick;
	si.video_render = text_scale_video_render;
	si.video_get_color_space = text_scale_get_color_space;
	return si;
}

struct obs_source_info text_scale_filter_info = init_source_info();

