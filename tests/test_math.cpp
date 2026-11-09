#include <iostream>
#include <iomanip>
#include <cassert>
#include <cmath>
#include <string>
#include <vector>
#include "../src/text-scale-math.h"

using namespace textscale;

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(cond, msg) \
	do { \
		tests_run++; \
		if (cond) { \
			tests_passed++; \
		} else { \
			tests_failed++; \
			std::cerr << " [FAIL] " << msg << " (" << #cond << ") at line " << __LINE__ << std::endl; \
		} \
	} while (0)

static bool ApproxEqual(double a, double b, double epsilon = 0.005)
{
	return std::abs(a - b) <= epsilon;
}

void TestPromptExactExamples()
{
	std::cout << "--- Running TestPromptExactExamples ---" << std::endl;
	const double target_w = 267.0;
	const double min_s = 0.65;
	const double max_s = 1.00;

	// 100 px -> 100% (1.000)
	double s100 = CalculateAutoCondenseScale(100.0, target_w, min_s, max_s);
	TEST_ASSERT(ApproxEqual(s100, 1.000), "100 px must yield 100% scale");

	// 180 px -> 100% (1.000)
	double s180 = CalculateAutoCondenseScale(180.0, target_w, min_s, max_s);
	TEST_ASSERT(ApproxEqual(s180, 1.000), "180 px must yield 100% scale");

	// 267 px -> 100% (1.000)
	double s267 = CalculateAutoCondenseScale(267.0, target_w, min_s, max_s);
	TEST_ASSERT(ApproxEqual(s267, 1.000), "267 px must yield 100% scale");

	// 300 px -> ~89% (267 / 300 = 0.890)
	double s300 = CalculateAutoCondenseScale(300.0, target_w, min_s, max_s);
	TEST_ASSERT(ApproxEqual(s300, 0.890), "300 px must yield 89.0% scale");

	// 350 px -> ~76.3% (267 / 350 = 0.762857...)
	double s350 = CalculateAutoCondenseScale(350.0, target_w, min_s, max_s);
	TEST_ASSERT(ApproxEqual(s350, 0.76286, 0.001), "350 px must yield ~76.3% scale");

	// 400 px -> 267 / 400 = 0.6675, with min 65% -> 0.6675 (66.75%)
	double s400 = CalculateAutoCondenseScale(400.0, target_w, min_s, max_s);
	TEST_ASSERT(ApproxEqual(s400, 0.6675), "400 px must yield 66.75% scale");

	// 500 px -> 267 / 500 = 0.534, clamped to min 65% (0.650)
	double s500 = CalculateAutoCondenseScale(500.0, target_w, min_s, max_s);
	TEST_ASSERT(ApproxEqual(s500, 0.650), "500 px must yield clamped minimum 65.0% scale");
}

void TestClampingLimits()
{
	std::cout << "--- Running TestClampingLimits ---" << std::endl;
	const double target_w = 200.0;

	// Custom min_scale = 0.50, max_scale = 0.90
	double s_short = CalculateAutoCondenseScale(100.0, target_w, 0.50, 0.90);
	TEST_ASSERT(ApproxEqual(s_short, 0.90), "Max scale 90% must limit short text if max_scale < 1.0");

	// Custom min_scale = 0.80
	double s_long = CalculateAutoCondenseScale(400.0, target_w, 0.80, 1.00);
	TEST_ASSERT(ApproxEqual(s_long, 0.80), "400 px with 80% min limit must clamp at 80%");

	// min_scale > max_scale inverted defense
	double s_inv = CalculateAutoCondenseScale(300.0, target_w, 0.90, 0.70);
	TEST_ASSERT(s_inv >= 0.70 && s_inv <= 0.90, "Inverted min/max scale must be defensively handled");
}

void TestZeroAndInvalidDimensions()
{
	std::cout << "--- Running TestZeroAndInvalidDimensions ---" << std::endl;

	// Zero content width
	double s_zero_w = CalculateAutoCondenseScale(0.0, 267.0, 0.65, 1.0);
	TEST_ASSERT(ApproxEqual(s_zero_w, 1.0), "Zero content width should default to 1.0");

	// Negative content width
	double s_neg_w = CalculateAutoCondenseScale(-150.0, 267.0, 0.65, 1.0);
	TEST_ASSERT(ApproxEqual(s_neg_w, 1.0), "Negative content width should default to 1.0");

	// Zero target width
	double s_zero_t = CalculateAutoCondenseScale(200.0, 0.0, 0.65, 1.0);
	TEST_ASSERT(ApproxEqual(s_zero_t, 0.65), "Zero target width must clamp to min_scale");

	// Negative target width
	double s_neg_t = CalculateAutoCondenseScale(200.0, -50.0, 0.65, 1.0);
	TEST_ASSERT(ApproxEqual(s_neg_t, 0.65), "Negative target width must clamp to min_scale");
}

void TestAlignmentCalculations()
{
	std::cout << "--- Running TestAlignmentCalculations ---" << std::endl;
	const double container_w = 267.0;

	// Content width 100 px (fits within 267 px)
	double off_left = CalculateAlignmentOffset(container_w, 100.0, Alignment::Left);
	TEST_ASSERT(ApproxEqual(off_left, 0.0), "Left alignment offset must be 0.0");

	double off_center = CalculateAlignmentOffset(container_w, 100.0, Alignment::Center);
	TEST_ASSERT(ApproxEqual(off_center, 83.5), "Center alignment offset for 100 px inside 267 px must be 83.5 px");

	double off_right = CalculateAlignmentOffset(container_w, 100.0, Alignment::Right);
	TEST_ASSERT(ApproxEqual(off_right, 167.0), "Right alignment offset for 100 px inside 267 px must be 167.0 px");

	// Content width exactly matching container
	double off_exact_c = CalculateAlignmentOffset(container_w, 267.0, Alignment::Center);
	TEST_ASSERT(ApproxEqual(off_exact_c, 0.0), "Center alignment for full-width content must be 0.0");

	// Overflow content (e.g. 325 px)
	double off_over_c = CalculateAlignmentOffset(container_w, 325.0, Alignment::Center);
	TEST_ASSERT(ApproxEqual(off_over_c, -29.0), "Center alignment for 325 px overflowing content must be -29.0 px");

	double off_over_r = CalculateAlignmentOffset(container_w, 325.0, Alignment::Right);
	TEST_ASSERT(ApproxEqual(off_over_r, -58.0), "Right alignment for 325 px overflowing content must be -58.0 px");
}

void TestTransformPipeline()
{
	std::cout << "--- Running TestTransformPipeline ---" << std::endl;

	ScaleConfig config;
	config.enable_auto_condense = true;
	config.target_width = 267.0;
	config.min_scale = 0.65;
	config.max_scale = 1.00;
	config.preserve_height = true;
	config.alignment = Alignment::Center;

	// Case 1: Short text 180 px
	ScaleResult res1 = CalculateScaleTransform(180, 50, 180.0, config);
	TEST_ASSERT(ApproxEqual(res1.scale_x, 1.0), "Transform: 180 px text scale_x must be 1.0");
	TEST_ASSERT(ApproxEqual(res1.scale_y, 1.0), "Transform: preserve_height must keep scale_y at 1.0");
	TEST_ASSERT(res1.output_width == 267, "Transform: output_width must be target_width (267)");
	TEST_ASSERT(res1.output_height == 50, "Transform: output_height must equal base_height (50)");
	TEST_ASSERT(ApproxEqual(res1.render_offset_x, (267.0 - 180.0) / 2.0), "Transform: offset_x must center 180 px in 267 px");

	// Case 2: Long text 350 px
	ScaleResult res2 = CalculateScaleTransform(350, 50, 350.0, config);
	TEST_ASSERT(ApproxEqual(res2.scale_x, 267.0 / 350.0, 0.001), "Transform: 350 px text scale_x must be 267/350");
	TEST_ASSERT(ApproxEqual(res2.condensed_content_width, 267.0, 0.001), "Transform: condensed width must equal target_width");
	TEST_ASSERT(ApproxEqual(res2.render_offset_x, 0.0, 0.001), "Transform: centered full-width text offset must be 0.0");

	// Case 3: Source dimension resize / content update
	ScaleResult res3 = CalculateScaleTransform(500, 60, 500.0, config);
	TEST_ASSERT(ApproxEqual(res3.scale_x, 0.65), "Transform: 500 px text must be clamped to min_scale 65%");
	TEST_ASSERT(res3.output_height == 60, "Transform: output_height must reflect new base_height (60)");

	// Case 4: Manual Scaling mode
	ScaleConfig manual_cfg;
	manual_cfg.enable_auto_condense = false;
	manual_cfg.enable_manual_scale = true;
	manual_cfg.manual_scale_x = 0.75;
	manual_cfg.manual_scale_y = 1.25;

	ScaleResult res_manual = CalculateScaleTransform(200, 100, 200.0, manual_cfg);
	TEST_ASSERT(ApproxEqual(res_manual.scale_x, 0.75), "Manual transform scale_x must be 0.75");
	TEST_ASSERT(ApproxEqual(res_manual.scale_y, 1.25), "Manual transform scale_y must be 1.25");
	TEST_ASSERT(res_manual.output_width == 150, "Manual transform output_width must be 200 * 0.75 = 150");
	TEST_ASSERT(res_manual.output_height == 125, "Manual transform output_height must be 100 * 1.25 = 125");
}

int main()
{
	std::cout << "========================================" << std::endl;
	std::cout << "TextScale Mathematical Logic Unit Tests" << std::endl;
	std::cout << "========================================" << std::endl;

	TestPromptExactExamples();
	TestClampingLimits();
	TestZeroAndInvalidDimensions();
	TestAlignmentCalculations();
	TestTransformPipeline();

	std::cout << "========================================" << std::endl;
	std::cout << "Summary: " << tests_passed << "/" << tests_run << " tests passed." << std::endl;
	if (tests_failed > 0) {
		std::cout << "FAILED: " << tests_failed << " test(s) failed!" << std::endl;
		return 1;
	}

	std::cout << "ALL TESTS PASSED SUCCESSFULLY!" << std::endl;
	return 0;
}
