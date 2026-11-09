#include <obs-module.h>
#include "text-scale-filter.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("text-scale", "en-US")

MODULE_EXPORT const char *obs_module_description(void)
{
	return "TextScale: Native OBS text condensing and scaling filter";
}

MODULE_EXPORT const char *obs_module_name(void)
{
	return "TextScale";
}

bool obs_module_load(void)
{
	blog(LOG_INFO, "[TextScale] Plugin version 1.0.0 loaded successfully");
	obs_register_source(&text_scale_filter_info);
	return true;
}

void obs_module_unload(void)
{
	blog(LOG_INFO, "[TextScale] Plugin unloaded");
}
