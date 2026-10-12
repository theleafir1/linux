// SPDX-License-Identifier: GPL-2.0-only
/*
 * Generated with linux-mdss-dsi-panel-driver-generator from vendor device tree:
 * Copyright (c) 2013, The Linux Foundation. All rights reserved.
 * Copyright (c) 2026 <ваше имя> <ваш@email>
 */

#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/regulator/consumer.h>

#include <drm/display/drm_dsc.h>
#include <drm/display/drm_dsc_helper.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <drm/drm_probe_helper.h>

struct luna21603_samsung_1080_2400_dsc_dvt_120hz {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct drm_dsc_config dsc;
	struct gpio_desc *reset_gpio;
	struct regulator_bulk_data *supplies;
};

/* При необходимости сверьте имена с dsi_panel_pwr_supply_vddr в вашем DTSI */
static const struct regulator_bulk_data luna21603_samsung_1080_2400_dsc_dvt_120hz_supplies[] = {
	{ .supply = "vddio" },	/* 1.8V */
	{ .supply = "vdd" },	/* 3.3V / vddr */
};

static inline struct luna21603_samsung_1080_2400_dsc_dvt_120hz *
to_luna21603_samsung_1080_2400_dsc_dvt_120hz(struct drm_panel *panel)
{
	return container_of(panel, struct luna21603_samsung_1080_2400_dsc_dvt_120hz, panel);
}

static void luna21603_samsung_1080_2400_dsc_dvt_120hz_reset(
		struct luna21603_samsung_1080_2400_dsc_dvt_120hz *ctx)
{
	/* DTS reset-sequence = <1 10>, <0 10>, <1 10> */
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(10000, 11000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 0);
	usleep_range(10000, 11000);
	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	usleep_range(10000, 11000);
}

static int luna21603_samsung_1080_2400_dsc_dvt_120hz_on(
		struct luna21603_samsung_1080_2400_dsc_dvt_120hz *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = dsi };

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	/* PPS Setting */
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x9e,
		0x11, 0x00, 0x00, 0x89, 0x30, 0x80, 0x09,
		0x60, 0x04, 0x38, 0x00, 0x1e, 0x02, 0x1c,
		0x02, 0x1c, 0x02, 0x00, 0x02, 0x0e, 0x00,
		0x20, 0x02, 0xe3, 0x00, 0x07, 0x00, 0x0c,
		0x03, 0x50, 0x03, 0x64, 0x18, 0x00, 0x10,
		0xf0, 0x03, 0x0c, 0x20, 0x00, 0x06, 0x0b,
		0x0b, 0x33, 0x0e, 0x1c, 0x2a, 0x38, 0x46,
		0x54, 0x62, 0x69, 0x70, 0x77, 0x79, 0x7b,
		0x7d, 0x7e, 0x01, 0x02, 0x01, 0x00, 0x09,
		0x40, 0x09, 0xbe, 0x19, 0xfc, 0x19, 0xfa,
		0x19, 0xf8, 0x1a, 0x38, 0x1a, 0x78, 0x1a,
		0xb6, 0x2a, 0xf6, 0x2b, 0x34, 0x2b, 0x74,
		0x3b, 0x74, 0x6b, 0xf4, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00);

	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xc2, 0x14);	/* MIC OFF */
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x9d, 0x01);	/* DSC enable */

	/* Err FG Set */
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xe5, 0x15);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xed, 0x44, 0x4c, 0x20);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xf0, 0xa5, 0xa5);

	/* Sleep Out (11h), ждём 120 мс */
	mipi_dsi_dcs_exit_sleep_mode_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 120);

	/* clk mask */
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb0, 0x2e, 0xf2);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xf2, 0x55);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xf0, 0xa5, 0xa5);

	/* CASET / PASET */
	mipi_dsi_dcs_set_column_address_multi(&dsi_ctx, 0x0000, 0x0437);
	mipi_dsi_dcs_set_page_address_multi(&dsi_ctx, 0x0000, 0x095f);

	/* FQ CON Setting */
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb0, 0x27, 0xf2);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xf2, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xf0, 0xa5, 0xa5);

	/* SEED CRC Setting (DCI-P3) */
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x5d, 0x86);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x62, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xb0, 0x2b, 0x62);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x62,
		0xe0, 0x00, 0x06, 0x13, 0xd3, 0x03, 0x07,
		0x0b, 0xee, 0x16, 0xfa, 0xe2, 0xf7, 0x00,
		0xe4, 0xe9, 0xe6, 0x03, 0xff, 0xff, 0xff);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xf0, 0xa5, 0xa5);

	/* FPS change: 120Hz */
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0x60, 0x08, 0x00);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xf7, 0x0f);
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xf0, 0xa5, 0xa5);

	/* TE On (Vsync) */
	mipi_dsi_dcs_set_tear_on_multi(&dsi_ctx, MIPI_DSI_DCS_TEAR_MODE_VBLANK);

	/* Display On (post-on-backlight из DTS) */
	mipi_dsi_dcs_write_seq_multi(&dsi_ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_dcs_set_display_on_multi(&dsi_ctx);
	mipi_dsi_usleep_range(&dsi_ctx, 1000, 2000);

	return dsi_ctx.accum_err;
}

static int luna21603_samsung_1080_2400_dsc_dvt_120hz_off(
		struct luna21603_samsung_1080_2400_dsc_dvt_120hz *ctx)
{
	struct mipi_dsi_device *dsi = ctx->dsi;
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = dsi };

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	mipi_dsi_dcs_set_display_off_multi(&dsi_ctx);
	mipi_dsi_usleep_range(&dsi_ctx, 10000, 11000);
	mipi_dsi_dcs_enter_sleep_mode_multi(&dsi_ctx);
	mipi_dsi_msleep(&dsi_ctx, 120);

	return dsi_ctx.accum_err;
}

static int luna21603_samsung_1080_2400_dsc_dvt_120hz_disable(struct drm_panel *panel)
{
	return luna21603_samsung_1080_2400_dsc_dvt_120hz_off(
			to_luna21603_samsung_1080_2400_dsc_dvt_120hz(panel));
}

static int luna21603_samsung_1080_2400_dsc_dvt_120hz_prepare(struct drm_panel *panel)
{
	struct luna21603_samsung_1080_2400_dsc_dvt_120hz *ctx =
			to_luna21603_samsung_1080_2400_dsc_dvt_120hz(panel);
	struct mipi_dsi_multi_context dsi_ctx = { .dsi = ctx->dsi };
	struct drm_dsc_picture_parameter_set pps;
	int ret;

	ret = regulator_bulk_enable(
			ARRAY_SIZE(luna21603_samsung_1080_2400_dsc_dvt_120hz_supplies),
			ctx->supplies);
	if (ret < 0)
		return ret;

	luna21603_samsung_1080_2400_dsc_dvt_120hz_reset(ctx);

	ret = luna21603_samsung_1080_2400_dsc_dvt_120hz_on(ctx);
	if (ret < 0) {
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		goto err;
	}

	drm_dsc_pps_payload_pack(&pps, &ctx->dsc);
	mipi_dsi_picture_parameter_set_multi(&dsi_ctx, &pps);
	mipi_dsi_compression_mode_ext_multi(&dsi_ctx, true,
					    MIPI_DSI_COMPRESSION_DSC, 0);

	mipi_dsi_msleep(&dsi_ctx, 28);

	if (dsi_ctx.accum_err < 0) {
		ret = dsi_ctx.accum_err;
		goto err;
	}

	return 0;

err:
	regulator_bulk_disable(
			ARRAY_SIZE(luna21603_samsung_1080_2400_dsc_dvt_120hz_supplies),
			ctx->supplies);
	return ret;
}

static int luna21603_samsung_1080_2400_dsc_dvt_120hz_unprepare(struct drm_panel *panel)
{
	struct luna21603_samsung_1080_2400_dsc_dvt_120hz *ctx =
			to_luna21603_samsung_1080_2400_dsc_dvt_120hz(panel);

	gpiod_set_value_cansleep(ctx->reset_gpio, 1);
	regulator_bulk_disable(
			ARRAY_SIZE(luna21603_samsung_1080_2400_dsc_dvt_120hz_supplies),
			ctx->supplies);

	return 0;
}

static const struct drm_display_mode luna21603_samsung_1080_2400_dsc_dvt_120hz_modes[] = {
	/* Первым идёт самый высокий режим */
	{ /* 120Hz */
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
		.type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED,
	},
	{ /* 60Hz */
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
		.type = DRM_MODE_TYPE_DRIVER,
	},
};

static int luna21603_samsung_1080_2400_dsc_dvt_120hz_get_modes(struct drm_panel *panel,
							       struct drm_connector *connector)
{
	int count = 0;

	for (int i = 0; i < ARRAY_SIZE(luna21603_samsung_1080_2400_dsc_dvt_120hz_modes); i++)
		count += drm_connector_helper_get_modes_fixed(connector,
				&luna21603_samsung_1080_2400_dsc_dvt_120hz_modes[i]);

	return count;
}

static const struct drm_panel_funcs luna21603_samsung_1080_2400_dsc_dvt_120hz_panel_funcs = {
	.prepare = luna21603_samsung_1080_2400_dsc_dvt_120hz_prepare,
	.unprepare = luna21603_samsung_1080_2400_dsc_dvt_120hz_unprepare,
	.disable = luna21603_samsung_1080_2400_dsc_dvt_120hz_disable,
	.get_modes = luna21603_samsung_1080_2400_dsc_dvt_120hz_get_modes,
};

static int luna21603_samsung_1080_2400_dsc_dvt_120hz_bl_update_status(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	u16 brightness = backlight_get_brightness(bl);
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_set_display_brightness_large(dsi, brightness);
	if (ret < 0)
		return ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	return 0;
}

static int luna21603_samsung_1080_2400_dsc_dvt_120hz_bl_get_brightness(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	u16 brightness;
	int ret;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_get_display_brightness_large(dsi, &brightness);
	if (ret < 0)
		return ret;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	return brightness;
}

static const struct backlight_ops luna21603_samsung_1080_2400_dsc_dvt_120hz_bl_ops = {
	.update_status = luna21603_samsung_1080_2400_dsc_dvt_120hz_bl_update_status,
	.get_brightness = luna21603_samsung_1080_2400_dsc_dvt_120hz_bl_get_brightness,
};

static struct backlight_device *
luna21603_samsung_1080_2400_dsc_dvt_120hz_create_backlight(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	const struct backlight_properties props = {
		.type = BACKLIGHT_RAW,
		.brightness = 2000,
		.max_brightness = 2784,
	};

	return devm_backlight_device_register(dev, dev_name(dev), dev, dsi,
			&luna21603_samsung_1080_2400_dsc_dvt_120hz_bl_ops, &props);
}

static int luna21603_samsung_1080_2400_dsc_dvt_120hz_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct luna21603_samsung_1080_2400_dsc_dvt_120hz *ctx;
	int ret;

	ctx = devm_drm_panel_alloc(dev,
			struct luna21603_samsung_1080_2400_dsc_dvt_120hz, panel,
			&luna21603_samsung_1080_2400_dsc_dvt_120hz_panel_funcs,
			DRM_MODE_CONNECTOR_DSI);
	if (IS_ERR(ctx))
		return PTR_ERR(ctx);

	ret = devm_regulator_bulk_get_const(dev,
			ARRAY_SIZE(luna21603_samsung_1080_2400_dsc_dvt_120hz_supplies),
			luna21603_samsung_1080_2400_dsc_dvt_120hz_supplies,
			&ctx->supplies);
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
			  MIPI_DSI_CLOCK_NON_CONTINUOUS;

	ctx->panel.prepare_prev_first = true;

	ctx->panel.backlight =
		luna21603_samsung_1080_2400_dsc_dvt_120hz_create_backlight(dsi);
	if (IS_ERR(ctx->panel.backlight))
		return dev_err_probe(dev, PTR_ERR(ctx->panel.backlight),
				     "Failed to create backlight\n");

	drm_panel_add(&ctx->panel);

	/* Эта панель поддерживает только DSC — включаем безусловно */
	dsi->dsc = &ctx->dsc;

	ctx->dsc.dsc_version_major = 1;
	ctx->dsc.dsc_version_minor = 1;
	ctx->dsc.slice_height = 30;
	ctx->dsc.slice_width = 540;
	ctx->dsc.slice_count = 1080 / ctx->dsc.slice_width;
	ctx->dsc.bits_per_component = 8;
	ctx->dsc.bits_per_pixel = 8 << 4; /* 4 дробных бита */
	ctx->dsc.block_pred_enable = true;

	ret = devm_mipi_dsi_attach(dev, dsi);
	if (ret < 0) {
		drm_panel_remove(&ctx->panel);
		return dev_err_probe(dev, ret, "Failed to attach to DSI host\n");
	}

	return 0;
}

static void luna21603_samsung_1080_2400_dsc_dvt_120hz_remove(struct mipi_dsi_device *dsi)
{
	struct luna21603_samsung_1080_2400_dsc_dvt_120hz *ctx =
			mipi_dsi_get_drvdata(dsi);

	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id luna21603_samsung_1080_2400_dsc_dvt_120hz_of_match[] = {
	{ .compatible = "samsung,ams643ye01" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, luna21603_samsung_1080_2400_dsc_dvt_120hz_of_match);

static struct mipi_dsi_driver luna21603_samsung_1080_2400_dsc_dvt_120hz_driver = {
	.probe = luna21603_samsung_1080_2400_dsc_dvt_120hz_probe,
	.remove = luna21603_samsung_1080_2400_dsc_dvt_120hz_remove,
	.driver = {
		.name = "panel-luna21603-samsung-1080-2400-dsc-dvt-120hz",
		.of_match_table = luna21603_samsung_1080_2400_dsc_dvt_120hz_of_match,
	},
};
module_mipi_dsi_driver(luna21603_samsung_1080_2400_dsc_dvt_120hz_driver);

MODULE_AUTHOR("<ваше имя> <ваш@email>");
MODULE_DESCRIPTION("DRM driver for Samsung AMS643YE01 DSC cmd mode panel");
MODULE_LICENSE("GPL");