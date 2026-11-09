#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace textscale {

enum class Alignment {
	Left = 0,
	Center = 1,
	Right = 2
};

struct ScaleConfig {
	bool enable_auto_condense = false;
	double target_width = 267.0; // pixels
	double min_scale = 0.65;    // 65%
	double max_scale = 1.00;    // 100%
	bool preserve_height = true;
	Alignment alignment = Alignment::Left;

	bool enable_manual_scale = false;
	double manual_scale_x = 1.0; // 10% - 100%
	double manual_scale_y = 1.0; // 10% - 200%

	bool high_quality = true;
	bool debug_info = false;
};

struct ScaleResult {
	double scale_x = 1.0;
	double scale_y = 1.0;
	double render_offset_x = 0.0;
	double render_offset_y = 0.0;
	uint32_t output_width = 0;
	uint32_t output_height = 0;
	double condensed_content_width = 0.0;
};

inline double Clamp(double val, double min_val, double max_val)
{
	if (min_val > max_val) {
		std::swap(min_val, max_val);
	}
	return std::max(min_val, std::min(max_val, val));
}

/**
 * Calculates the horizontal condensing scale factor.
 * Rules:
 *  - Short text or text that fits within target_width remains at 100% (or max_scale).
 *  - Never automatically enlarge text beyond 1.0 / max_scale.
 *  - Scale = target_width / measured_content_width when exceeding target_width.
 *  - Clamped to [min_scale, max_scale].
 *  - If text cannot fit without exceeding min_scale, retain min_scale and allow overflow.
 */
inline double CalculateAutoCondenseScale(double measured_content_width,
					 double target_width,
					 double min_scale = 0.65,
					 double max_scale = 1.00)
{
	if (min_scale <= 0.0)
		min_scale = 0.01;
	if (max_scale < min_scale)
		max_scale = min_scale;

	// Zero or negative input content width
	if (measured_content_width <= 0.0) {
		return Clamp(1.0, min_scale, max_scale);
	}

	// Zero or negative target width
	if (target_width <= 0.0) {
		return min_scale;
	}

	// If text width <= target width: never enlarge, keep 100% (or max_scale if max < 1.0)
	if (measured_content_width <= target_width) {
		return std::min(1.0, max_scale);
	}

	double raw_scale = target_width / measured_content_width;
	if (raw_scale >= 1.0) {
		return std::min(1.0, max_scale);
	}

	return Clamp(raw_scale, min_scale, max_scale);
}

/**
 * Calculates the horizontal offset for Left, Center, or Right alignment.
 * Alignment is calculated using the condensed text width inside the target width container.
 */
inline double CalculateAlignmentOffset(double container_width,
				       double content_width,
				       Alignment alignment)
{
	switch (alignment) {
	case Alignment::Center:
		return (container_width - content_width) / 2.0;
	case Alignment::Right:
		return container_width - content_width;
	case Alignment::Left:
	default:
		return 0.0;
	}
}

/**
 * Computes full scaling transformation and output dimensions.
 */
inline ScaleResult CalculateScaleTransform(uint32_t base_width,
					   uint32_t base_height,
					   double measured_content_width,
					   const ScaleConfig &config)
{
	ScaleResult result;

	if (base_width == 0 || base_height == 0) {
		return result;
	}

	double content_w = (measured_content_width > 0.0) ? measured_content_width : (double)base_width;

	if (config.enable_auto_condense) {
		result.scale_x = CalculateAutoCondenseScale(
			content_w,
			config.target_width,
			config.min_scale,
			config.max_scale);

		result.scale_y = config.preserve_height
			? 1.0
			: (config.enable_manual_scale ? Clamp(config.manual_scale_y, 0.10, 2.0) : 1.0);

		result.condensed_content_width = content_w * result.scale_x;

		uint32_t target_w = (config.target_width > 0.0) ? (uint32_t)std::round(config.target_width) : base_width;
		if (target_w < 1)
			target_w = 1;

		result.output_width = target_w;
		result.output_height = (uint32_t)std::round(base_height * result.scale_y);

		result.render_offset_x = CalculateAlignmentOffset(
			(double)result.output_width,
			result.condensed_content_width,
			config.alignment);
		result.render_offset_y = 0.0;
	} else if (config.enable_manual_scale) {
		result.scale_x = Clamp(config.manual_scale_x, 0.10, 1.0);
		result.scale_y = Clamp(config.manual_scale_y, 0.10, 2.0);

		result.condensed_content_width = (double)base_width * result.scale_x;
		result.output_width = (uint32_t)std::round((double)base_width * result.scale_x);
		result.output_height = (uint32_t)std::round((double)base_height * result.scale_y);

		result.render_offset_x = 0.0;
		result.render_offset_y = 0.0;
	} else {
		result.scale_x = 1.0;
		result.scale_y = 1.0;
		result.condensed_content_width = (double)base_width;
		result.output_width = base_width;
		result.output_height = base_height;
		result.render_offset_x = 0.0;
		result.render_offset_y = 0.0;
	}

	if (result.output_width < 1)
		result.output_width = 1;
	if (result.output_height < 1)
		result.output_height = 1;

	return result;
}

} // namespace textscale
