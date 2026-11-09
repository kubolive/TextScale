#include "text-measurement.h"
#include <util/platform.h>

#ifdef _WIN32
#include <windows.h>
#include <gdiplus.h>
#endif

namespace textscale {

#ifdef _WIN32
static ULONG_PTR gdiplus_token = 0;
static int gdiplus_ref_count = 0;

static void EnsureGdiplusInitialized()
{
	if (gdiplus_ref_count++ == 0) {
		Gdiplus::GdiplusStartupInput input;
		Gdiplus::GdiplusStartup(&gdiplus_token, &input, nullptr);
	}
}

static void ReleaseGdiplus()
{
	if (--gdiplus_ref_count == 0 && gdiplus_token) {
		Gdiplus::GdiplusShutdown(gdiplus_token);
		gdiplus_token = 0;
	}
}
#endif

TextMeasurement::TextMeasurement()
{
#ifdef _WIN32
	EnsureGdiplusInitialized();
#endif
}

TextMeasurement::~TextMeasurement()
{
#ifdef _WIN32
	ReleaseGdiplus();
#endif
}

void TextMeasurement::InvalidateCache()
{
	has_cache_ = false;
}

bool TextMeasurement::MeasureGdiplusSource(obs_source_t *target, obs_data_t *settings, double &out_width)
{
	if (!target || !settings)
		return false;

	bool use_extents = obs_data_get_bool(settings, "extents");
	uint32_t base_w = obs_source_get_base_width(target);
	uint32_t base_h = obs_source_get_base_height(target);

	// If custom extents are NOT enabled in GDI+ text source,
	// the base width is already the exact measured rendered text width.
	if (!use_extents) {
		out_width = (double)base_w;
		cached_width_ = out_width;
		has_cache_ = true;
		last_base_width_ = base_w;
		last_base_height_ = base_h;
		last_use_extents_ = false;
		return true;
	}

#ifdef _WIN32
	// Custom extents ARE enabled. The canvas is fixed (e.g. 500 px),
	// but the text inside only occupies its typographic bounding box.
	const char *raw_text = obs_data_get_string(settings, "text");
	if (!raw_text)
		raw_text = "";

	obs_data_t *font_obj = obs_data_get_obj(settings, "font");
	const char *face = font_obj ? obs_data_get_string(font_obj, "face") : "Arial";
	int font_size = font_obj ? (int)obs_data_get_int(font_obj, "size") : 32;
	int font_flags = font_obj ? (int)obs_data_get_int(font_obj, "flags") : 0;
	bool use_outline = obs_data_get_bool(settings, "outline");
	int outline_size = (int)obs_data_get_int(settings, "outline_size");

	// Check if cached measurement is still valid
	if (has_cache_ && last_use_extents_ && last_text_ == raw_text && last_font_face_ == face &&
	    last_font_size_ == font_size && last_font_flags_ == font_flags &&
	    last_use_outline_ == use_outline && last_outline_size_ == outline_size) {
		obs_data_release(font_obj);
		out_width = cached_width_;
		return true;
	}

	if (strlen(raw_text) == 0) {
		obs_data_release(font_obj);
		out_width = 0.0;
		cached_width_ = 0.0;
		has_cache_ = true;
		return true;
	}

	// Convert UTF-8 text to wide string
	int wlen = MultiByteToWideChar(CP_UTF8, 0, raw_text, -1, nullptr, 0);
	if (wlen <= 0) {
		obs_data_release(font_obj);
		out_width = (double)base_w;
		return false;
	}

	std::wstring wtext(wlen, 0);
	MultiByteToWideChar(CP_UTF8, 0, raw_text, -1, &wtext[0], wlen);

	int wface_len = MultiByteToWideChar(CP_UTF8, 0, face, -1, nullptr, 0);
	std::wstring wface(wface_len > 0 ? wface_len : 1, 0);
	if (wface_len > 0)
		MultiByteToWideChar(CP_UTF8, 0, face, -1, &wface[0], wface_len);

	INT style = Gdiplus::FontStyleRegular;
	if (font_flags & 1) // Bold
		style |= Gdiplus::FontStyleBold;
	if (font_flags & 2) // Italic
		style |= Gdiplus::FontStyleItalic;
	if (font_flags & 4) // Underline
		style |= Gdiplus::FontStyleUnderline;
	if (font_flags & 8) // Strikeout
		style |= Gdiplus::FontStyleStrikeout;

	Gdiplus::FontFamily family(wface.c_str());
	Gdiplus::Font font(&family, (Gdiplus::REAL)font_size, style, Gdiplus::UnitPixel);

	Gdiplus::Bitmap dummy(1, 1, PixelFormat32bppARGB);
	Gdiplus::Graphics graphics(&dummy);

	Gdiplus::StringFormat format(Gdiplus::StringFormat::GenericTypographic());
	format.SetFormatFlags(format.GetFormatFlags() | Gdiplus::StringFormatFlagsNoFitBlackBox |
			      Gdiplus::StringFormatFlagsLineLimit);

	Gdiplus::RectF bounding_box;
	Gdiplus::PointF origin(0.0f, 0.0f);
	Gdiplus::Status stat = graphics.MeasureString(wtext.c_str(), -1, &font, origin, &format, &bounding_box);

	obs_data_release(font_obj);

	if (stat == Gdiplus::Ok) {
		double measured_w = (double)bounding_box.Width;
		if (use_outline && outline_size > 0) {
			measured_w += (double)outline_size;
		}

		out_width = measured_w;
		cached_width_ = out_width;
		has_cache_ = true;
		last_text_ = raw_text;
		last_font_face_ = face;
		last_font_size_ = font_size;
		last_font_flags_ = font_flags;
		last_use_outline_ = use_outline;
		last_outline_size_ = outline_size;
		last_use_extents_ = true;
		last_base_width_ = base_w;
		last_base_height_ = base_h;
		return true;
	}
#else
	obs_data_release(font_obj);
#endif

	out_width = (double)base_w;
	return false;
}

bool TextMeasurement::MeasureTargetContentWidth(obs_source_t *target, double &out_width, bool &out_is_gdiplus)
{
	out_is_gdiplus = false;
	if (!target) {
		out_width = 0.0;
		return false;
	}

	const char *source_id = obs_source_get_id(target);
	if (source_id && (strcmp(source_id, "text_gdiplus") == 0 ||
			  strcmp(source_id, "text_gdiplus_v2") == 0 ||
			  strcmp(source_id, "text_gdiplus_v3") == 0)) {
		out_is_gdiplus = true;
		obs_data_t *settings = obs_source_get_settings(target);
		bool success = MeasureGdiplusSource(target, settings, out_width);
		obs_data_release(settings);
		if (success)
			return true;
	}

	// Fallback to base source dimensions for generic or FreeType 2 sources
	uint32_t base_w = obs_source_get_base_width(target);
	out_width = (double)base_w;
	return true;
}

} // namespace textscale
