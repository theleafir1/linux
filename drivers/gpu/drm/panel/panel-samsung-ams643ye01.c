// SPDX-License-Identifier: GPL-2.0-only
/*
 * Samsung AMS643YE01 1080x2400 DSC command-mode AMOLED panel driver
 *
 * Ported from the downstream Qualcomm/OPLUS panel dtsi
 * "samsung AMS643YE01 dsc cmd mode panel" (luna21603, DVT, 120/60 Hz).
 *
 * Not ported (no mainline infrastructure / not needed for basic bring-up):
 * ESD register check, AOD (lp1/nolp), HBM/FOD, SEED colour modes,
 * OSC clock modes, DC dimming, ADFR/qsync, dynamic DSI clock, ULPS.
 */

#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/regulator/consumer.h>
#include <linux/string.h>

#include <video/mipi_display.h>

#include <drm/display/drm_dsc.h>
#include <drm/display/drm_dsc_helper.h>
#include <drm/drm_connector.h>
#include <drm/drm_crtc.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>

#define AMS643YE01_MAX_BRIGHTNESS	2047	/* "bl-normal-max-level" */
#define AMS643YE01_DEFAULT_BRIGHTNESS	1024

struct ams643ye01 {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct drm_dsc_config dsc;
	struct gpio_desc *reset_gpio;
	struct regulator_bulk_data supplies[3];
	struct backlight_device *bl;
	struct drm_connector *connector;	/* remembered from get_modes() */
	bool enabled;
};

static inline struct ams643ye01 *to_ams643ye01(struct drm_panel *panel)
{
	return container_of(panel, struct ams643ye01, panel);
}

/* Samsung "level 2" test key */
#define ams643ye01_unlock(ctx) \
	mipi_dsi_dcs_write_seq_multi(ctx, 0xf0, 0x5a, 0x5a)
#define ams643ye01_lock(ctx) \
	mipi_dsi_dcs_write_seq_multi(ctx, 0xf0, 0xa5, 0xa5)

static void ams643ye01_reset(struct ams643ye01 *ctx)
{
	/* downstream: <1 10>, <0 10>, <1 10> (physical levels), gpio is ACTIVE_LOW */
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(10000, 11000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(10000, 11000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(10000, 11000);
}

/*
 * drm_panel_funcs::prepare() has no idea about the mode being set, so look
 * at the CRTC state of the connector. Fall back to 120 Hz.
 */
static bool ams643ye01_want_60hz(struct ams643ye01 *ctx)
{
	struct drm_connector *connector = ctx->connector;
	struct drm_crtc_state *crtc_state;

	if (!connector || !connector->state || !connector->state->crtc) {
		dev_warn_once(&ctx->dsi->dev,
			      "no connector/crtc state in prepare, assuming 120 Hz\n");
		return false;
	}

	crtc_state = connector->state->crtc->state;
	if (!crtc_state)
		return false;

	return drm_mode_vrefresh(&crtc_state->adjusted_mode) == 60;
}

static int ams643ye01_on(struct ams643ye01 *ctx, bool fps60)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = dsi };
	struct drm_dsc_picture_parameter_set pps;
	u8 pps_cmd[1 + sizeof(pps)];

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;	/* on-command-state = dsi_lp_mode */

	/* PPS: Samsung wants it as DCS long write 0x9e (128 bytes) */
	drm_dsc_pps_payload_pack(&pps, &ctx->dsc);
	pps_cmd[0] = 0x9e;
	memcpy(&pps_cmd[1], &pps, sizeof(pps));
	mipi_dsi_dcs_write_buffer_multi(&dsi_ctx, pps_cmd, sizeof(pps_cmd));

	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc2, 0x14);	/* MIC off */
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x9d, 0x01);	/* DSC enable */

	/* Err FG set */
	ams643ye01_unlock(&dsi_ctx);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xe5, 0x15);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xed, 0x44, 0x4c, 0x20);
	ams643ye01_lock(&dsi_ctx);

	mipi_dsi_dcs_exit_sleep_mode_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 120);

	/* clk mask */
	ams643ye01_unlock(&dsi_ctx);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb0, 0x2e, 0xf2);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xf2, 0x55);
	ams643ye01_lock(&dsi_ctx);

	/* CASET / PASET */
	mipi_dsi_dcs_set_column_address_multi(&dsi_ctx, 0, 1080 - 1);
	mipi_dsi_dcs_set_page_address_multi(&dsi_ctx, 0, 2400 - 1);

	/* FQ CON */
	ams643ye01_unlock(&dsi_ctx);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb0, 0x27, 0xf2);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xf2, 0x00);
	ams643ye01_lock(&dsi_ctx);

	/* SEED CRC on, DCI-P3 */
	ams643ye01_unlock(&dsi_ctx);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x5d, 0x86);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x62, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb0, 0x2b, 0x62);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x62,
				     0xe0, 0x00, 0x06, 0x13, 0xd3, 0x03, 0x07,
				     0x0b, 0xee, 0x16, 0xfa, 0xe2, 0xf7, 0x00,
				     0xe4, 0xe9, 0xe6, 0x03, 0xff, 0xff, 0xff);
	ams643ye01_lock(&dsi_ctx);

	/* FPS select */
	ams643ye01_unlock(&dsi_ctx);
	if (fps60)
		mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x60, 0x00, 0x00);
	else
		mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x60, 0x08, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xf7, 0x0f);
	ams643ye01_lock(&dsi_ctx);

	/* TE on (vsync) */
	mipi_dsi_dcs_set_tear_on_multi(&dsi_ctx, MIPI_DSI_DCS_TEAR_MODE_VBLANK);

	return dsi_ctx.accum_err;
}

static int ams643ye01_send_brightness(struct ams643ye01 *ctx, u16 brightness)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = dsi };
	unsigned long old_flags = dsi->mode_flags;
	/* DBV is big endian on this panel ("51 07 FF" == 2047 in downstream dtsi) */
	u8 buf[3] = { MIPI_DCS_SET_DISPLAY_BRIGHTNESS, brightness >> 8,
		      brightness & 0xff };

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;	/* backlight is sent in HS */
	mipi_dsi_dcs_write_buffer_multi(&dsi_ctx, buf, sizeof(buf));
	dsi->mode_flags = old_flags;

	return dsi_ctx.accum_err;
}

static int ams643ye01_prepare(struct drm_panel *panel)
{
	struct ams643ye01 *ctx = to_ams643ye01(panel);
	struct device *dev = &ctx->dsi->dev;
	int ret;

	ret = regulator_bulk_enable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
	if (ret < 0) {
		dev_err(dev, "Failed to enable regulators: %d\n", ret);
		return ret;
	}

	ams643ye01_reset(ctx);

	ret = ams643ye01_on(ctx, ams643ye01_want_60hz(ctx));
	if (ret < 0) {
		dev_err(dev, "Failed to initialize panel: %d\n", ret);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
		return ret;
	}

	return 0;
}

static int ams643ye01_enable(struct drm_panel *panel)
{
	struct ams643ye01 *ctx = to_ams643ye01(panel);
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };
	int ret;

	/* BCTRL on, HBM off (matches "53 20" used by downstream) */
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, MIPI_DCS_WRITE_CONTROL_DISPLAY, 0x20);
	if (dsi_ctx.accum_err)
		return dsi_ctx.accum_err;

	ret = ams643ye01_send_brightness(ctx, backlight_get_brightness(ctx->bl));
	if (ret < 0)
		return ret;

	/* post-on-backlight: 20 ms, then display on */
	msleep(20);
	mipi_dsi_dcs_set_display_on_multi(&dsi_ctx);
	if (dsi_ctx.accum_err)
		return dsi_ctx.accum_err;

	ctx->enabled = true;

	return 0;
}

static int ams643ye01_disable(struct drm_panel *panel)
{
	struct ams643ye01 *ctx = to_ams643ye01(panel);
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };

	ctx->enabled = false;

	mipi_dsi_dcs_set_display_off_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 10);

	return dsi_ctx.accum_err;
}

static int ams643ye01_unprepare(struct drm_panel *panel)
{
	struct ams643ye01 *ctx = to_ams643ye01(panel);
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };

	mipi_dsi_dcs_enter_sleep_mode_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 120);
	if (dsi_ctx.accum_err)
		dev_err(&ctx->dsi->dev, "Failed to enter sleep mode: %d\n",
			dsi_ctx.accum_err);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);

	return 0;
}

/*
 * Command mode panel: porches only matter for pixel clock / transfer time
 * calculation in the DSI host and DPU. Values are taken from downstream.
 */
static const struct drm_display_mode ams643ye01_modes[] = {
	{	/* timing@0: 120 Hz */
		.clock = (1080 + 50 + 10 + 40) * (2400 + 21 + 2 + 9) * 120 / 1000,
		.hdisplay = 1080,
		.hsync_start = 1080 + 50,
		.hsync_end = 1080 + 50 + 10,
		.htotal = 1080 + 50 + 10 + 40,
		.vdisplay = 2400,
		.vsync_start = 2400 + 21,
		.vsync_end = 2400 + 21 + 2,
		.vtotal = 2400 + 21 + 2 + 9,
		.width_mm = 67,
		.height_mm = 149,
	},
	{	/* timing@1: 60 Hz */
		.clock = (1080 + 50 + 10 + 40) * (2400 + 21 + 2 + 10) * 60 / 1000,
		.hdisplay = 1080,
		.hsync_start = 1080 + 50,
		.hsync_end = 1080 + 50 + 10,
		.htotal = 1080 + 50 + 10 + 40,
		.vdisplay = 2400,
		.vsync_start = 2400 + 21,
		.vsync_end = 2400 + 21 + 2,
		.vtotal = 2400 + 21 + 2 + 10,
		.width_mm = 67,
		.height_mm = 149,
	},
};

static int ams643ye01_get_modes(struct drm_panel *panel,
				struct drm_connector *connector)
{
	struct ams643ye01 *ctx = to_ams643ye01(panel);
	int i;

	ctx->connector = connector;

	for (i = 0; i < ARRAY_SIZE(ams643ye01_modes); i++) {
		struct drm_display_mode *mode;

		mode = drm_mode_duplicate(connector->dev, &ams643ye01_modes[i]);
		if (!mode)
			return i ? i : -ENOMEM;

		mode->type = DRM_MODE_TYPE_DRIVER;
		if (i == 0)
			mode->type |= DRM_MODE_TYPE_PREFERRED;
		drm_mode_set_name(mode);
		drm_mode_probed_add(connector, mode);
	}

	connector->display_info.width_mm = ams643ye01_modes[0].width_mm;
	connector->display_info.height_mm = ams643ye01_modes[0].height_mm;

	return ARRAY_SIZE(ams643ye01_modes);
}

static const struct drm_panel_funcs ams643ye01_panel_funcs = {
	.prepare = ams643ye01_prepare,
	.enable = ams643ye01_enable,
	.disable = ams643ye01_disable,
	.unprepare = ams643ye01_unprepare,
	.get_modes = ams643ye01_get_modes,
};

static int ams643ye01_bl_update_status(struct backlight_device *bl)
{
	struct ams643ye01 *ctx = bl_get_data(bl);

	/* applied from .enable() once the panel is on */
	if (!ctx->enabled)
		return 0;

	return ams643ye01_send_brightness(ctx, backlight_get_brightness(bl));
}

static const struct backlight_ops ams643ye01_bl_ops = {
	.update_status = ams643ye01_bl_update_status,
};

static int ams643ye01_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct backlight_properties props = {
		.type = BACKLIGHT_RAW,
		.brightness = AMS643YE01_DEFAULT_BRIGHTNESS,
		.max_brightness = AMS643YE01_MAX_BRIGHTNESS,
	};
	struct ams643ye01 *ctx;
	int ret;

	ctx = devm_drm_panel_alloc(dev, struct ams643ye01, panel,
				   &ams643ye01_panel_funcs,
				   DRM_MODE_CONNECTOR_DSI);
	if (IS_ERR(ctx))
		return PTR_ERR(ctx);

	ctx->supplies[0].supply = "vdd";
	ctx->supplies[1].supply = "vddio";
	ctx->supplies[2].supply = "dvdd";
	ret = devm_regulator_bulk_get(dev, ARRAY_SIZE(ctx->supplies),
				      ctx->supplies);
	if (ret < 0)
		return dev_err_probe(dev, ret, "Failed to get regulators\n");

	ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio))
		return dev_err_probe(dev, PTR_ERR(ctx->reset_gpio),
				     "Failed to get reset-gpios\n");

	ctx->dsi = dsi;
	mipi_dsi_set_drvdata(dsi, ctx);

	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_NO_EOT_PACKET |
			  MIPI_DSI_CLOCK_NON_CONTINUOUS |
			  MIPI_DSI_MODE_LPM;

	/* The host must be up before we can send DCS in prepare() */
	ctx->panel.prepare_prev_first = true;

	ctx->bl = devm_backlight_device_register(dev, dev_name(dev), dev, ctx,
						 &ams643ye01_bl_ops, &props);
	if (IS_ERR(ctx->bl))
		return dev_err_probe(dev, PTR_ERR(ctx->bl),
				     "Failed to register backlight\n");

	/*
	 * DSC 1.1, 8 bpc, 8 bpp (3:1), 2 slices of 540x30, block prediction.
	 * The remaining RC parameters are filled in by the DSI host.
	 */
	ctx->dsc.dsc_version_major = 1;
	ctx->dsc.dsc_version_minor = 1;
	ctx->dsc.slice_height = 30;
	ctx->dsc.slice_width = 540;
	ctx->dsc.slice_count = 2;
	ctx->dsc.bits_per_component = 8;
	ctx->dsc.bits_per_pixel = 8 << 4;	/* 4 fractional bits */
	ctx->dsc.block_pred_enable = true;
	dsi->dsc = &ctx->dsc;

	drm_panel_add(&ctx->panel);

	ret = devm_mipi_dsi_attach(dev, dsi);
	if (ret < 0) {
		drm_panel_remove(&ctx->panel);
		return dev_err_probe(dev, ret, "Failed to attach to DSI host\n");
	}

	return 0;
}

static void ams643ye01_remove(struct mipi_dsi_device *dsi)
{
	struct ams643ye01 *ctx = mipi_dsi_get_drvdata(dsi);

	ctx->connector = NULL;
	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id ams643ye01_of_match[] = {
	{ .compatible = "samsung,ams643ye01" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ams643ye01_of_match);

static struct mipi_dsi_driver ams643ye01_driver = {
	.probe = ams643ye01_probe,
	.remove = ams643ye01_remove,
	.driver = {
		.name = "panel-samsung-ams643ye01",
		.of_match_table = ams643ye01_of_match,
	},
};
module_mipi_dsi_driver(ams643ye01_driver);

MODULE_DESCRIPTION("DRM driver for Samsung AMS643YE01 DSC cmd mode AMOLED panel");
MODULE_LICENSE("GPL");