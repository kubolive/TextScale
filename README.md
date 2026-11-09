# TextScale – Native OBS Studio Text Condensing Filter

**TextScale** is a high-performance, production-quality native C++ video filter plugin for OBS Studio (OBS 30.x / 31.x+ on Windows 64-bit).

Designed specifically for broadcast graphics operators, live newsrooms, and esports productions, TextScale provides horizontal text scaling and automatic text condensation for native OBS text sources (`text_gdiplus_v3`). It enables news tickers, lower-thirds, breaking news headlines, and name supers to fit fixed-width graphic containers without reducing font size, distorting vertical height, or causing clipping.

---

## Key Features

- **Genuine Native C++ OBS Video Filter**: Registers directly into the OBS filter architecture (`OBS_SOURCE_TYPE_FILTER`). Accessible via:  
  `OBS Studio → Select Source → Filters → Add Filter (+) → TextScale`.
- **Auto Condense (Horizontal-Only)**:
  - Text fitting within the target container remains at 100% original width.
  - Text exceeding the target width is automatically condensed horizontally:  
    $$\text{scale}_x = \min\left(1.0, \frac{\text{target\_width}}{\text{measured\_width}}\right)$$
  - Never enlarges text automatically.
  - Strictly preserves original text height and font size.
  - Clamps to a user-defined minimum scale (e.g. 65%) with controlled overflow beyond the threshold.
- **Manual Scaling**:
  - Independent Horizontal Scale: 10% to 100%.
  - Independent Vertical Scale: 10% to 200%.
- **Container Alignment**:
  - Left, Center, and Right alignment calculated using the **final condensed text width**, keeping text properly centered or right-aligned within target bounding areas.
- **Accurate Content Measurement**:
  - Interrogates `text_gdiplus_v3` data settings to measure visible glyph content widths accurately even when custom source bounding extents are enabled.
  - Internal dimension caching avoids per-frame CPU measurement overhead.
- **Hardware-Accelerated GPU Rendering**:
  - Custom OBS GPU effect shader (`text_scale.effect`) running on Direct3D 11 / OpenGL.
  - Preserves alpha transparency, anti-aliasing, font shadows, and text outlines.
  - Border clamping (`BorderColor = 00000000`) prevents edge pixel bleeding and distortion.

---

## Filter Properties & UI Groups

TextScale provides an intuitive, responsive property panel divided into four functional groups:

### Group A – Scaling (Manual)
| Property | Type | Range | Default | Description |
|:---|:---|:---|:---|:---|
| **Enable Manual Scaling** | Boolean | On / Off | `Off` | Toggles manual scale overrides. |
| **Horizontal Scale** | Integer Slider | 10% – 100% | `100%` | Horizontal compression percentage. Original height is preserved. |
| **Vertical Scale** | Integer Slider | 10% – 200% | `100%` | Vertical scaling percentage. Independent of horizontal scale. |
| **Reset Scaling** | Button | Action | — | Resets horizontal and vertical scales to 100%. |

### Group B – Auto Condense (Primary Feature)
| Property | Type | Range | Default | Description |
|:---|:---|:---|:---|:---|
| **Enable Auto Condense** | Boolean | On / Off | `Off` | Enables automated content-aware width condensation. |
| **Target Width** | Integer Spinbox | 10 – 3840 px | `267 px` | The width of the graphic container/lower-third. |
| **Minimum Scale** | Integer Slider | 10% – 100% | `65%` | Compression floor. Text will not be condensed narrower than this scale. |
| **Maximum Scale** | Integer Slider | 10% – 100% | `100%` | Maximum scale cap (defaults to 100% to prevent enlargement). |
| **Preserve Height** | Boolean | On / Off | `On` | Locks source vertical height regardless of width compression. |

### Group C – Alignment
| Property | Type | Range | Default | Description |
|:---|:---|:---|:---|:---|
| **Alignment** | List | Left / Center / Right | `Left` | Positions condensed text within the container based on its final scaled width. |

### Group D – Advanced
| Property | Type | Range | Default | Description |
|:---|:---|:---|:---|:---|
| **High Quality Scaling** | Boolean | On / Off | `On` | Uses bilinear texture sampling with transparent boundary clamping. |
| **Debug Information** | Boolean | On / Off | `Off` | Outputs live scaling metrics and offsets to OBS log file. |
| **Reset to Defaults** | Button | Action | — | Restores all settings to default factory values. |

---

## Text Source Compatibility Analysis

### 1. Primary Target: Text (GDI+) (`text_gdiplus_v3`)
- **Status:** **Fully Supported & Verified**.
- **Platform:** Windows 10 / Windows 11 x64.
- **Implementation:** GDI+ is the native, default text source on Windows in OBS Studio 30 & 31. TextScale reads the underlying font specification (family, point size, bold/italic style, outline width) and text string directly from the source's `obs_data_t` settings. It computes typographic content width using the Windows GDI+ subsystem and caches the result until the text string or font properties change. Custom bounding extents (`use_extents`) and word wrap modes are handled correctly without measuring trailing blank canvas space.

### 2. Secondary Target: Text (FreeType 2) (`text_ft2_source`)
- **Status:** **Deprecated in OBS Windows / Fallback Mode**.
- **Analysis:** In OBS Studio 31.1.2 on Windows x64, FreeType 2 is marked as deprecated and omitted from standard source creation dialogs. When FreeType 2 sources are encountered (e.g. imported from Linux/macOS scene collections), TextScale gracefully falls back to using the base source rendered texture dimensions (`obs_source_get_base_width`).
- **Policy Compliance:** The plugin does not force-enable or modify deprecated OBS core components, nor does it link against FreeType 2 libraries.

---

## Directory Structure

```
text_scale_obs/
├── CMakeLists.txt              # CMake build configuration
├── README.md                   # Plugin documentation
├── BUILD.md                    # Build guide and compiler prerequisites
├── INSTALL.md                  # Installation and deployment guide
├── TEST_REPORT.md              # Detailed verification and test results
├── data/
│   ├── text_scale.effect       # Direct3D/OpenGL HLSL GPU shader
│   └── locale/
│       └── en-US.ini           # English localization strings
├── src/
│   ├── plugin-main.cpp         # OBS module registration & exports
│   ├── text-scale-filter.h     # Filter class definition & struct declarations
│   ├── text-scale-filter.cpp   # Filter lifecycle, properties UI, and GPU render callbacks
│   ├── text-scale-math.h       # Pure math core (auto-condense & alignment algorithms)
│   ├── text-measurement.h     # GDI+ typography measuring interface
│   └── text-measurement.cpp   # GDI+ Win32 measurement implementation & cache
└── tests/
    ├── test_math.cpp           # Standalone unit tests (34 test cases)
    ├── test_obs_integration.py # Automated live OBS launch and render test
    └── test_functional_matrix.py # Comprehensive functional verification in OBS
```

---

## Known Behaviors & Boundary Cases

1. **Extreme Compression Clamping**: If a title is exceptionally long (e.g., 900+ px in a 267 px box), the scale is clamped at `min_scale` (65%), allowing the text to safely overflow rather than illegibly squishing or reducing font size.
2. **Empty Text String**: When a text source has no text or 0 px width, the scale calculations safely default to 1.0 (100%) and 0 offset, avoiding division by zero.
3. **Vietnamese & Unicode Diacritics**: Fully tested with complex Vietnamese diacritic characters (e.g. `BẢN TIN DỰ BÁO THỜI TIẾT ĐẶC BIỆT`). Measurement correctly includes combining accents and descenders without clipping.
4. **Outlines and Drop Shadows**: Outline padding is factored into the measurement to prevent clipping around the leftmost and rightmost glyph edges.

---

## License

Developed for native OBS Studio environments under GNU General Public License v2 (GPL-2.0) matching OBS Studio plugin standards.
