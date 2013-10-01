/*
Copyright 2012 Canonical Ltd.

Authors:
    Charles Kerr <charles.kerr@canonical.com>

This program is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License version 3, as published
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranties of
MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <gio/gio.h>
#include <gtest/gtest.h>
#include "device.h"
#include "service.h"

class DeviceTest : public ::testing::Test
{
  private:

    guint log_handler_id;

    int log_count_ipower_actual;

    static void log_count_func (const gchar    * log_domain G_GNUC_UNUSED,
                                GLogLevelFlags   log_level  G_GNUC_UNUSED,
                                const gchar    * message    G_GNUC_UNUSED,
                                gpointer         gself)
    {
      reinterpret_cast<DeviceTest*>(gself)->log_count_ipower_actual++;
    }

  protected:

    int log_count_ipower_expected;

  protected:

    virtual void SetUp()
    {
      const GLogLevelFlags flags = GLogLevelFlags(G_LOG_LEVEL_CRITICAL|G_LOG_LEVEL_WARNING);
      log_handler_id = g_log_set_handler ("Indicator-Power", flags, log_count_func, this);
      log_count_ipower_expected = 0;
      log_count_ipower_actual = 0;
    }

    virtual void TearDown()
    {
      ASSERT_EQ (log_count_ipower_expected, log_count_ipower_actual);
      g_log_remove_handler ("Indicator-Power", log_handler_id);
    }

  protected:

    void check_icon_names (const IndicatorPowerDevice * device, const char * expected)
    {
      char ** names = indicator_power_device_get_icon_names (device);
      char * str = g_strjoinv (";", names);
      ASSERT_STREQ (expected, str);
      g_free (str);
      g_strfreev (names);
    }

    void check_label (const IndicatorPowerDevice * device,
                      const char * expected_label)
    {
      char * label;

      label = indicator_power_device_get_label (device);
      EXPECT_STREQ (expected_label, label);

      g_free (label);
    }

    void check_header (const IndicatorPowerDevice * device,
                       const char * expected_time_and_percent,
                       const char * expected_time,
                       const char * expected_percent,
                       const char * expected_a11y)
    {
      char * label;
      char * a11y;

      indicator_power_device_get_header (device, true, true, &label, &a11y);
      EXPECT_STREQ (expected_time_and_percent, label);
      EXPECT_STREQ (expected_a11y, a11y);
      g_free (label);
      g_free (a11y);

      indicator_power_device_get_header (device, true, false, &label, &a11y);
      EXPECT_STREQ (expected_time, label);
      EXPECT_STREQ (expected_a11y, a11y);
      g_free (label);
      g_free (a11y);

      indicator_power_device_get_header (device, false, true, &label, &a11y);
      EXPECT_STREQ (expected_percent, label);
      EXPECT_STREQ (expected_a11y, a11y);
      g_free (label);
      g_free (a11y);

      indicator_power_device_get_header (device, false, false, &label, &a11y);
      ASSERT_TRUE (!label || !*label);
      EXPECT_STREQ (expected_a11y, a11y);
      g_free (label);
      g_free (a11y);
    }
};

/***
****
***/

TEST_F(DeviceTest, GObjectNew)
{
  GObject * o = G_OBJECT (g_object_new (INDICATOR_POWER_DEVICE_TYPE, NULL));
  ASSERT_TRUE (o != NULL);
  ASSERT_TRUE (INDICATOR_IS_POWER_DEVICE(o));
  g_object_run_dispose (o); // used to get coverage of both branches in the object's dispose func's g_clear_*() calls
  g_object_unref (o);
}

TEST_F(DeviceTest, Properties)
{
  int i;
  gdouble d;
  GObject * o;
  gchar * str;
  guint64 u64;
  const gchar * key;

  o = G_OBJECT (g_object_new (INDICATOR_POWER_DEVICE_TYPE, NULL));
  ASSERT_TRUE (o != NULL);
  ASSERT_TRUE (INDICATOR_IS_POWER_DEVICE(o));

  // Test getting & setting a Device's properties.

  // KIND
  key = INDICATOR_POWER_DEVICE_KIND;
  g_object_set (o, key, UP_DEVICE_KIND_BATTERY, NULL);
  g_object_get (o, key, &i, NULL);
  ASSERT_EQ (i, UP_DEVICE_KIND_BATTERY);

  // STATE
  key = INDICATOR_POWER_DEVICE_STATE;
  g_object_set (o, key, UP_DEVICE_STATE_CHARGING, NULL);
  g_object_get (o, key, &i, NULL);
  ASSERT_EQ (i, UP_DEVICE_STATE_CHARGING);

  // OBJECT_PATH
  key = INDICATOR_POWER_DEVICE_OBJECT_PATH;
  g_object_set (o, key, "/object/path", NULL);
  g_object_get (o, key, &str, NULL);
  ASSERT_STREQ (str, "/object/path");
  g_free (str);

  // PERCENTAGE
  key = INDICATOR_POWER_DEVICE_PERCENTAGE;
  g_object_set (o, key, 50.0, NULL);
  g_object_get (o, key, &d, NULL);
  ASSERT_EQ((int)d, 50);

  // TIME
  key = INDICATOR_POWER_DEVICE_TIME;
  g_object_set (o, key, (guint64)30, NULL);
  g_object_get (o, key, &u64, NULL);
  ASSERT_EQ(u64, 30);

  // cleanup
  g_object_unref (o);
}

TEST_F(DeviceTest, New)
{
  IndicatorPowerDevice * device = indicator_power_device_new ("/object/path",
                                                              UP_DEVICE_KIND_BATTERY,
                                                              50.0,
                                                              UP_DEVICE_STATE_CHARGING,
                                                              30);
  ASSERT_TRUE (device != NULL);
  ASSERT_TRUE (INDICATOR_IS_POWER_DEVICE(device));
  ASSERT_EQ (indicator_power_device_get_kind(device), UP_DEVICE_KIND_BATTERY);
  ASSERT_EQ (indicator_power_device_get_state(device), UP_DEVICE_STATE_CHARGING);
  ASSERT_STREQ (indicator_power_device_get_object_path(device), "/object/path");
  ASSERT_EQ ((int)indicator_power_device_get_percentage(device), 50);
  ASSERT_EQ (indicator_power_device_get_time(device), 30);

  // cleanup
  g_object_unref (device);
}

TEST_F(DeviceTest, NewFromVariant)
{
  GVariant * variant = g_variant_new ("(susdut)",
                                      "/object/path",
                                      (guint32) UP_DEVICE_KIND_BATTERY,
                                      "icon",
                                      (gdouble) 50.0,
                                      (guint32) UP_DEVICE_STATE_CHARGING,
                                      (guint64) 30);
  IndicatorPowerDevice * device = indicator_power_device_new_from_variant (variant);
  ASSERT_TRUE (variant != NULL);
  ASSERT_TRUE (device != NULL);
  ASSERT_TRUE (INDICATOR_IS_POWER_DEVICE(device));
  ASSERT_EQ (indicator_power_device_get_kind(device), UP_DEVICE_KIND_BATTERY);
  ASSERT_EQ (indicator_power_device_get_state(device), UP_DEVICE_STATE_CHARGING);
  ASSERT_STREQ (indicator_power_device_get_object_path(device), "/object/path");
  ASSERT_EQ ((int)indicator_power_device_get_percentage(device), 50);
  ASSERT_EQ (indicator_power_device_get_time(device), 30);

  // cleanup
  g_object_unref (device);
  g_variant_unref (variant);
}

TEST_F(DeviceTest, BadAccessors)
{
  // test that these functions can handle being passed NULL pointers
  IndicatorPowerDevice * device = NULL;
  indicator_power_device_get_kind (device);
  indicator_power_device_get_time (device);
  indicator_power_device_get_state (device);
  indicator_power_device_get_percentage (device);
  indicator_power_device_get_object_path (device);
  log_count_ipower_expected += 5;

  // test that these functions can handle being passed non-device GObjects
  device = reinterpret_cast<IndicatorPowerDevice*>(g_cancellable_new ());
  indicator_power_device_get_kind (device);
  indicator_power_device_get_time (device);
  indicator_power_device_get_state (device);
  indicator_power_device_get_percentage (device);
  indicator_power_device_get_object_path (device);
  log_count_ipower_expected += 5;

  g_object_unref (device);
}

/***
****
***/

TEST_F(DeviceTest, IconNames)
{
  IndicatorPowerDevice * device = INDICATOR_POWER_DEVICE (g_object_new (INDICATOR_POWER_DEVICE_TYPE, NULL));
  GObject * o = G_OBJECT(device);

  // bad arguments
  log_count_ipower_expected++;
  ASSERT_TRUE (indicator_power_device_get_icon_names (NULL) == NULL);

  // power
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_LINE_POWER,
                   NULL);
  check_icon_names (device, "ac-adapter-symbolic;"
                            "ac-adapter");

  // monitor
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_MONITOR,
                   NULL);
  check_icon_names (device, "gpm-monitor-symbolic;"
                            "gpm-monitor");

  // devices that hold a charge
  struct {
    int kind;
    const gchar * kind_str;
  } devices[] = {
    { UP_DEVICE_KIND_BATTERY, "battery" },
    { UP_DEVICE_KIND_UPS, "ups" },
    { UP_DEVICE_KIND_MOUSE, "mouse" },
    { UP_DEVICE_KIND_KEYBOARD, "keyboard" },
    { UP_DEVICE_KIND_PHONE, "phone" }
  };

  GString * expected = g_string_new (NULL);
  for (int i=0, n=G_N_ELEMENTS(devices); i<n; i++)
    {
      const int kind = devices[i].kind;
      const gchar * kind_str = devices[i].kind_str;

      // empty
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_EMPTY,
                       NULL);

      g_string_append_printf (expected, "%s-empty-symbolic;", kind_str);
      g_string_append_printf (expected, "gpm-%s-empty;", kind_str);
      g_string_append_printf (expected, "gpm-%s-000;", kind_str);
      g_string_append_printf (expected, "%s-empty", kind_str);
      check_icon_names (device, expected->str);
      g_string_truncate (expected, 0);

      // charged
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_FULLY_CHARGED,
                       NULL);
      g_string_append_printf (expected, "%s-full-charged-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-full-charging-symbolic;", kind_str);
      g_string_append_printf (expected, "gpm-%s-full;", kind_str);
      g_string_append_printf (expected, "gpm-%s-100;", kind_str);
      g_string_append_printf (expected, "%s-full-charged;", kind_str);
      g_string_append_printf (expected, "%s-full-charging", kind_str);
      check_icon_names (device, expected->str);
      g_string_truncate (expected, 0);

      // charging, 95%
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_CHARGING,
                       INDICATOR_POWER_DEVICE_PERCENTAGE, 95.0,
                       NULL);

      g_string_append_printf (expected, "%s-full-charging-symbolic;", kind_str);
      g_string_append_printf (expected, "gpm-%s-100-charging;", kind_str);
      g_string_append_printf (expected, "%s-full-charging", kind_str);
      check_icon_names (device, expected->str);
      g_string_truncate (expected, 0);

      // charging, 85%
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_CHARGING,
                       INDICATOR_POWER_DEVICE_PERCENTAGE, 85.0,
                       NULL);
      g_string_append_printf (expected, "%s-full-charging-symbolic;", kind_str);
      g_string_append_printf (expected, "gpm-%s-080-charging;", kind_str);
      g_string_append_printf (expected, "%s-full-charging", kind_str);
      check_icon_names (device, expected->str);
      g_string_truncate (expected, 0);

      // charging, 50%
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_CHARGING,
                       INDICATOR_POWER_DEVICE_PERCENTAGE, 50.0,
                       NULL);
      g_string_append_printf (expected, "%s-good-charging-symbolic;", kind_str);
      g_string_append_printf (expected, "gpm-%s-060-charging;", kind_str);
      g_string_append_printf (expected, "%s-good-charging", kind_str);
      check_icon_names (device, expected->str);
      g_string_truncate (expected, 0);

      // charging, 25%
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_CHARGING,
                       INDICATOR_POWER_DEVICE_PERCENTAGE, 25.0,
                       NULL);
      g_string_append_printf (expected, "%s-low-charging-symbolic;", kind_str);
      g_string_append_printf (expected, "gpm-%s-020-charging;", kind_str);
      g_string_append_printf (expected, "%s-low-charging", kind_str);
      check_icon_names (device, expected->str);
      g_string_truncate (expected, 0);

      // charging, 5%
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_CHARGING,
                       INDICATOR_POWER_DEVICE_PERCENTAGE, 5.0,
                       NULL);
      g_string_append_printf (expected, "%s-caution-charging-symbolic;", kind_str);
      g_string_append_printf (expected, "gpm-%s-000-charging;", kind_str);
      g_string_append_printf (expected, "%s-caution-charging", kind_str);
      check_icon_names (device, expected->str);
      g_string_truncate (expected, 0);

      // discharging, 95%
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                       INDICATOR_POWER_DEVICE_PERCENTAGE, 95.0,
                   NULL);
      g_string_append_printf (expected, "%s-100;", kind_str);
      g_string_append_printf (expected, "gpm-%s-100;", kind_str);
      g_string_append_printf (expected, "%s-full-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-full", kind_str);
      check_icon_names (device, expected->str);
      g_string_truncate (expected, 0);

      // discharging, 85%
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                       INDICATOR_POWER_DEVICE_PERCENTAGE, 85.0,
                       NULL);
      g_string_append_printf (expected, "%s-080;", kind_str);
      g_string_append_printf (expected, "gpm-%s-080;", kind_str);
      g_string_append_printf (expected, "%s-full-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-full", kind_str);
      check_icon_names (device, expected->str);
      g_string_truncate (expected, 0);

      // discharging, 50% -- 1 hour left
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                       INDICATOR_POWER_DEVICE_PERCENTAGE, 50.0,
                       INDICATOR_POWER_DEVICE_TIME, (guint64)(60*60),
                       NULL);
      g_string_append_printf (expected, "%s-060;", kind_str);
      g_string_append_printf (expected, "gpm-%s-060;", kind_str);
      g_string_append_printf (expected, "%s-good-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-good", kind_str);
      check_icon_names (device, expected->str);
      g_string_truncate (expected, 0);

      // discharging, 25% -- 1 hour left
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                       INDICATOR_POWER_DEVICE_PERCENTAGE, 25.0,
                       INDICATOR_POWER_DEVICE_TIME, (guint64)(60*60),
                       NULL);
      g_string_append_printf (expected, "%s-040;", kind_str);
      g_string_append_printf (expected, "gpm-%s-040;", kind_str);
      g_string_append_printf (expected, "%s-good-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-good", kind_str);
      check_icon_names (device, expected->str);
      g_string_truncate (expected, 0);

      // discharging, 25% -- 15 minutes left
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                       INDICATOR_POWER_DEVICE_PERCENTAGE, 25.0,
                       INDICATOR_POWER_DEVICE_TIME, (guint64)(60*15),
                       NULL);
      g_string_append_printf (expected, "%s-020;", kind_str);
      g_string_append_printf (expected, "gpm-%s-020;", kind_str);
      g_string_append_printf (expected, "%s-low-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-low", kind_str);
      check_icon_names (device, expected->str);
      g_string_truncate (expected, 0);

      // discharging, 5% -- 1 hour left
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                       INDICATOR_POWER_DEVICE_PERCENTAGE, 5.0,
                       INDICATOR_POWER_DEVICE_TIME, (guint64)(60*60),
                   NULL);
      g_string_append_printf (expected, "%s-040;", kind_str);
      g_string_append_printf (expected, "gpm-%s-040;", kind_str);
      g_string_append_printf (expected, "%s-good-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-good", kind_str);
      check_icon_names (device, expected->str);
      g_string_truncate (expected, 0);

      // discharging, 5% -- 15 minutes left
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                       INDICATOR_POWER_DEVICE_PERCENTAGE, 5.0,
                       INDICATOR_POWER_DEVICE_TIME, (guint64)(60*15),
                       NULL);
      g_string_append_printf (expected, "%s-000;", kind_str);
      g_string_append_printf (expected, "gpm-%s-000;", kind_str);
      g_string_append_printf (expected, "%s-caution-symbolic;", kind_str);
      g_string_append_printf (expected, "%s-caution", kind_str);
      check_icon_names (device, expected->str);
      g_string_truncate (expected, 0);

      // state unknown
      g_object_set (o, INDICATOR_POWER_DEVICE_KIND, kind,
                       INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_UNKNOWN,
                       NULL);
      g_string_append_printf (expected, "%s-missing-symbolic;", kind_str);
      g_string_append_printf (expected, "gpm-%s-missing;", kind_str);
      g_string_append_printf (expected, "%s-missing", kind_str);
      check_icon_names (device, expected->str);
      g_string_truncate (expected, 0);
  }
  g_string_free (expected, TRUE);

  // cleanup
  g_object_unref(o);
}


TEST_F(DeviceTest, Labels)
{
  // set our language so that i18n won't break these tests
  char * real_lang = g_strdup(g_getenv ("LANG"));
  g_setenv ("LANG", "en_US.UTF-8", TRUE);

  // bad args: NULL device
  log_count_ipower_expected += 5;
  check_label (NULL, NULL);
  check_header (NULL, NULL, NULL, NULL, NULL);

  // bad args: a GObject that isn't a device
  log_count_ipower_expected += 5;
  GObject * o = G_OBJECT(g_cancellable_new());
  check_label ((IndicatorPowerDevice*)o, NULL);
  check_header (NULL, NULL, NULL, NULL, NULL);
  g_object_unref (o);

  /**
  ***
  **/

  IndicatorPowerDevice * device = INDICATOR_POWER_DEVICE (g_object_new (INDICATOR_POWER_DEVICE_TYPE, NULL));
  o = G_OBJECT(device);

  // charging
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_BATTERY,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_CHARGING,
                   INDICATOR_POWER_DEVICE_PERCENTAGE, 50.0,
                   INDICATOR_POWER_DEVICE_TIME, guint64(60*61),
                   NULL);
  check_label (device, "Battery (1:01 to charge)");
  check_header (device, "(1:01, 50%)",
                        "(1:01)",
                        "(50%)",
                        "Battery (1 hour 1 minute to charge, 50%)");

  // discharging, < 12 hours left
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_BATTERY,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                   INDICATOR_POWER_DEVICE_PERCENTAGE, 50.0,
                   INDICATOR_POWER_DEVICE_TIME, guint64(60*61),
                   NULL);
  check_label (device, "Battery (1:01 left)");
  check_header (device, "(1:01, 50%)",
                        "(1:01)",
                        "(50%)",
                        "Battery (1 hour 1 minute left, 50%)");

  // discharging, > 24 hours left
  // we don't show the clock time when > 24 hours discharging
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_BATTERY,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                   INDICATOR_POWER_DEVICE_PERCENTAGE, 50.0,
                   INDICATOR_POWER_DEVICE_TIME, guint64(60*60*25),
                   NULL);
  check_label (device, "Battery");
  check_header (device, "(50%)",
                        "",
                        "(50%)",
                        "Battery (50%)");

// fully charged
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_BATTERY,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_FULLY_CHARGED,
                   INDICATOR_POWER_DEVICE_PERCENTAGE, 100.0,
                   INDICATOR_POWER_DEVICE_TIME, guint64(0),
                   NULL);
  check_label (device, "Battery (charged)");
  check_header (device, "(100%)",
                        "",
                        "(100%)",
                        "Battery (charged, 100%)");

  // percentage but no time estimate
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_BATTERY,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                   INDICATOR_POWER_DEVICE_PERCENTAGE, 50.0,
                   INDICATOR_POWER_DEVICE_TIME, guint64(0),
                   NULL);
  check_label (device, "Battery (estimating…)");
  check_header (device, "(estimating…, 50%)",
                        "(estimating…)",
                        "(50%)",
                        "Battery (estimating…, 50%)");

  // no percentage, no time estimate
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_BATTERY,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_DISCHARGING,
                   INDICATOR_POWER_DEVICE_PERCENTAGE, 0.0,
                   INDICATOR_POWER_DEVICE_TIME, guint64(0),
                   NULL);
  check_label (device, "Battery (not present)");
  check_header (device, "", "", "", "Battery (not present)");

  // power line
  g_object_set (o, INDICATOR_POWER_DEVICE_KIND, UP_DEVICE_KIND_LINE_POWER,
                   INDICATOR_POWER_DEVICE_STATE, UP_DEVICE_STATE_UNKNOWN,
                   INDICATOR_POWER_DEVICE_PERCENTAGE, 0.0,
                   INDICATOR_POWER_DEVICE_TIME, guint64(0),
                   NULL);
  check_label (device, "AC Adapter");
  check_header (device, "", "", "", "AC Adapter");

  // cleanup
  g_object_unref(o);
  g_setenv ("LANG", real_lang, TRUE);
  g_free (real_lang);
}

/* The menu title should tell you at a glance what you need to know most: what
   device will lose power soonest (and optionally when), or otherwise which
   device will take longest to charge (and optionally how long it will take). */
TEST_F(DeviceTest, ChoosePrimary)
{
  GList * device_list;
  IndicatorPowerDevice * a;
  IndicatorPowerDevice * b;

  a = indicator_power_device_new ("/org/freedesktop/UPower/devices/mouse",
                                  UP_DEVICE_KIND_MOUSE,
                                  0.0,
                                  UP_DEVICE_STATE_DISCHARGING,
                                  0);
  b = indicator_power_device_new ("/org/freedesktop/UPower/devices/battery",
                                  UP_DEVICE_KIND_BATTERY,
                                  0.0,
                                  UP_DEVICE_STATE_DISCHARGING,
                                  0);

  /* device states + time left to {discharge,charge} + % of charge left,
     sorted in order of preference wrt the spec's criteria.
     So tests[i] should be picked over any test with an index greater than i */
  struct {
    int kind;
    int state;
    guint64 time;
    double percentage;
  } tests[] = {
    { UP_DEVICE_KIND_BATTERY,    UP_DEVICE_STATE_DISCHARGING,   49,  50.0 },
    { UP_DEVICE_KIND_BATTERY,    UP_DEVICE_STATE_DISCHARGING,   50,  50.0 },
    { UP_DEVICE_KIND_BATTERY,    UP_DEVICE_STATE_DISCHARGING,   50, 100.0 },
    { UP_DEVICE_KIND_BATTERY,    UP_DEVICE_STATE_DISCHARGING,   51,  50.0 },
    { UP_DEVICE_KIND_BATTERY,    UP_DEVICE_STATE_CHARGING,      50,  50.0 },
    { UP_DEVICE_KIND_BATTERY,    UP_DEVICE_STATE_CHARGING,      49,  50.0 },
    { UP_DEVICE_KIND_BATTERY,    UP_DEVICE_STATE_CHARGING,      49, 100.0 },
    { UP_DEVICE_KIND_BATTERY,    UP_DEVICE_STATE_CHARGING,      48,  50.0 },
    { UP_DEVICE_KIND_BATTERY,    UP_DEVICE_STATE_FULLY_CHARGED,  0,  50.0 },
    { UP_DEVICE_KIND_KEYBOARD,   UP_DEVICE_STATE_FULLY_CHARGED,  0,  50.0 },
    { UP_DEVICE_KIND_LINE_POWER, UP_DEVICE_STATE_UNKNOWN,        0,   0.0 }
  };

  device_list = NULL;
  device_list = g_list_append (device_list, a);
  device_list = g_list_append (device_list, b);

  for (int i=0, n=G_N_ELEMENTS(tests); i<n; i++)
    {
      for (int j=i+1; j<n; j++)
        {
          g_object_set (a, INDICATOR_POWER_DEVICE_KIND, tests[i].kind,
                           INDICATOR_POWER_DEVICE_STATE, tests[i].state,
                           INDICATOR_POWER_DEVICE_TIME, guint64(tests[i].time),
                           INDICATOR_POWER_DEVICE_PERCENTAGE, tests[i].percentage,
                           NULL);
          g_object_set (b, INDICATOR_POWER_DEVICE_KIND, tests[j].kind,
                           INDICATOR_POWER_DEVICE_STATE, tests[j].state,
                           INDICATOR_POWER_DEVICE_TIME, guint64(tests[j].time),
                           INDICATOR_POWER_DEVICE_PERCENTAGE, tests[j].percentage,
                           NULL);
          ASSERT_EQ (a, indicator_power_service_choose_primary_device(device_list));

          /* reverse the list to check that list order doesn't matter */
          device_list = g_list_reverse (device_list);
          ASSERT_EQ (a, indicator_power_service_choose_primary_device(device_list));
        }
    }

  // cleanup
  g_list_free_full (device_list, g_object_unref);
}