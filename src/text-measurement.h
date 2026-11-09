#pragma once

#include <obs-module.h>
#include <string>
#include <cstdint>

namespace textscale {

class TextMeasurement {
public:
	TextMeasurement();
	~TextMeasurement();

	/**
	 * Determines the visible content width of the target source.
	 * Returns true if measurement succeeded and out_width was set.
	 * If the source is a GDI+ text source with custom extents,
	 * it accurately calculates the unpadded text width using GDI+ typography.
	 * If extents is disabled, returns obs_source_get_base_width(target).
	 */
	bool MeasureTargetContentWidth(obs_source_t *target, double &out_width, bool &out_is_gdiplus);

	/**
	 * Invalidate cached measurements (called when source settings change or dimension changes).
	 */
	void InvalidateCache();

private:
	bool MeasureGdiplusSource(obs_source_t *target, obs_data_t *settings, double &out_width);

	// Cached state to prevent redundant recalculation
	bool has_cache_ = false;
	double cached_width_ = 0.0;
	uint32_t last_base_width_ = 0;
	uint32_t last_base_height_ = 0;
	std::string last_text_;
	std::string last_font_face_;
	int last_font_size_ = 0;
	int last_font_flags_ = 0;
	int last_outline_size_ = 0;
	bool last_use_outline_ = false;
	bool last_use_extents_ = false;
};

} // namespace textscale
