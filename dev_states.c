#include "./dev_states.h"
#include <stdio.h>

static gint comp_ap_by_strength(gpointer a, gpointer b) {
  NMAccessPoint *ap_a = *(NMAccessPoint **)a;
  NMAccessPoint *ap_b = *(NMAccessPoint **)b;
  guint8 strength_a = nm_access_point_get_strength(ap_a);
  guint8 strength_b = nm_access_point_get_strength(ap_b);
  return strength_b - strength_a;
}

static void connect_ap_cb(NMClient *client, GAsyncResult *res,
                          DevStates *dev_states) {
  dev_states_check_swap(dev_states);
}

static void notify_strength_cb(NMAccessPoint *ap, GParamSpec *spec,
                               DevStates *dev_states) {
  GBytes *ssid = nm_access_point_get_ssid(ap);
  guint8 strength = nm_access_point_get_strength(ap);
  g_debug("strength of the AP changed(ssid: %s, strength: %d)\n",
          (char *)g_bytes_get_data(ssid, NULL), strength);
  dev_states_check_swap(dev_states);

  NMAccessPoint *main_ap =
      dev_states->mdev != NULL
          ? nm_device_wifi_get_active_access_point(dev_states->mdev)
          : NULL;
  if (main_ap == ap && dev_states->init_mdev_ap_strength * 0.8 >= strength) {
    g_debug("strength of mdev's AP is decreasing\n");
    dev_states_request_scan(dev_states);
  }
}

void dev_states_check_swap(DevStates *dev_states) {
  g_debug("checking whether need to swap device role\n");
  NMAccessPoint *main_ap =
      dev_states->mdev != NULL
          ? nm_device_wifi_get_active_access_point(dev_states->mdev)
          : NULL;
  guint8 main_strength =
      main_ap != NULL ? nm_access_point_get_strength(main_ap) : 0;
  for (int i = 0; i < dev_states->devs->len; i++) {
    NMDeviceWifi *device = dev_states->devs->pdata[i];
    if (device == dev_states->mdev) {
      continue;
    }

    NMAccessPoint *ap = nm_device_wifi_get_active_access_point(device);
    if (ap == NULL) {
      continue;
    }

    guint8 strength = nm_access_point_get_strength(ap);
    GBytes *ssid = nm_access_point_get_ssid(ap);
    if (strength > main_strength) {
      g_debug(
          "strength of the sdev's AP (ssid: %s, strength: %d) is higher now\n",
          (char *)g_bytes_get_data(ssid, NULL), strength);
      NMDeviceWifi *old_mdev = dev_states->mdev;
      if (old_mdev != NULL) {
        NMAccessPoint *old_mdev_ap =
            nm_device_wifi_get_active_access_point(old_mdev);
        g_signal_handler_disconnect(old_mdev_ap, dev_states->mdev_handler_id);
      }

      dev_states->mdev = device;
      dev_states->init_mdev_ap_strength = strength;
      dev_states->mdev_handler_id =
          g_signal_connect(device, "notify::strength",
                           (GCallback)notify_strength_cb, dev_states);
      const char *iface = nm_device_get_iface((NMDevice *)device);
      g_debug("mdev is changed to the device(iface: %s)\n", iface);
    }
  }
}

GPtrArray *nm_device_wifi_get_available_aps(NMDeviceWifi *device) {
  const GPtrArray *connections =
      nm_device_get_available_connections((NMDevice *)device);
  const GPtrArray *aps = nm_device_wifi_get_access_points(device);
  GPtrArray *filtered_aps = g_ptr_array_new();
  for (int i = 0; i < aps->len; i++) {
    NMAccessPoint *ap = aps->pdata[i];
    GPtrArray *matched_conns =
        nm_access_point_filter_connections(ap, connections);
    if (matched_conns->len > 0) {
      g_ptr_array_add(filtered_aps, ap);
    }

    g_ptr_array_unref(matched_conns);
  }

  return filtered_aps;
}

void nm_device_wifi_connect_ap(NMDeviceWifi *device, NMClient *client,
                               NMAccessPoint *ap, GAsyncReadyCallback cb,
                               void *user_data) {
  const GPtrArray *connections =
      nm_device_get_available_connections((NMDevice *)device);
  GPtrArray *matched_conns =
      nm_access_point_filter_connections(ap, connections);
  if (matched_conns->len > 0) {
    NMConnection *conn = matched_conns->pdata[0];
    const char *dev_path = nm_object_get_path((NMObject *)device);
    nm_client_activate_connection_async(client, conn, (NMDevice *)device,
                                        dev_path, NULL, cb, user_data);
  }

  g_ptr_array_unref(matched_conns);
}

NMAccessPoint *dev_states_find_dev_ap(DevStates *dev_states,
                                      NMDeviceWifi *device, GPtrArray *aps) {
  if (aps->len <= 0) {
    return NULL;
  }

  if (dev_states->mdev == NULL) {
    return aps->pdata[0];
  }

  NMAccessPoint *cur_ap = nm_device_wifi_get_active_access_point(device);
  guint8 cur_ap_strength =
      cur_ap != NULL ? nm_access_point_get_strength(cur_ap) : 0;
  const char *cur_ap_bssid =
      cur_ap != NULL ? nm_access_point_get_bssid(cur_ap) : NULL;

  // calc threshold
  NMAccessPoint *main_ap =
      nm_device_wifi_get_active_access_point(dev_states->mdev);
  guint8 threshold = nm_access_point_get_strength(main_ap) * 0.8;
  const char *main_bssid = nm_access_point_get_bssid(main_ap);
  for (int i = 0; i < aps->len; i++) {
    NMAccessPoint *ap = aps->pdata[i];
    const char *ap_bssid = nm_access_point_get_bssid(ap);
    guint8 ap_strength = nm_access_point_get_strength(ap);
    if (cur_ap != NULL && strcmp(cur_ap_bssid, ap_bssid) != 0 &&
            ap_strength > cur_ap_strength ||
        cur_ap == NULL && strcmp(main_bssid, ap_bssid) != 0 &&
            ap_strength >= threshold) {
      return ap;
    }
  }

  return cur_ap == NULL ? aps->pdata[0] : NULL;
}

void dev_states_scan_cb(NMDeviceWifi *device, GAsyncResult *res,
                        DevStates *dev_states) {
  GPtrArray *aps = nm_device_wifi_get_available_aps(device);
  const char *iface = nm_device_get_iface((NMDevice *)device);
  g_debug("scan completed. %d APs found on the device(iface: %s)\n", aps->len,
          iface);
  if (aps->len <= 0) {
    g_ptr_array_unref(aps);
    return;
  }

  // sort APs by strength
  g_ptr_array_sort(aps, (GCompareFunc)comp_ap_by_strength);
  NMAccessPoint *cur_ap = nm_device_wifi_get_active_access_point(device);
  NMAccessPoint *ap = dev_states_find_dev_ap(dev_states, device, aps);
  const char *cur_ap_bssid =
      cur_ap != NULL ? nm_access_point_get_bssid(cur_ap) : NULL;
  const char *ap_bssid = ap != NULL ? nm_access_point_get_bssid(ap) : NULL;
  if (ap != NULL && (cur_ap == NULL || strcmp(cur_ap_bssid, ap_bssid) != 0)) {
    const char *iface = nm_device_get_iface((NMDevice *)device);
    GBytes *ssid = nm_access_point_get_ssid(ap);
    g_debug("trying to connect to an AP(iface: %s, ssid: %s)\n", iface,
            (char *)g_bytes_get_data(ssid, NULL));
    nm_device_wifi_connect_ap(device, dev_states->client, ap,
                              (GAsyncReadyCallback)connect_ap_cb, dev_states);
  }

  g_ptr_array_unref(aps);
}

void dev_states_request_scan(DevStates *dev_states) {
  g_debug("scanning\n");
  for (int i = 0; i < dev_states->devs->len; i++) {
    NMDeviceWifi *device = dev_states->devs->pdata[i];
    nm_device_wifi_request_scan_async(
        device, NULL, (GAsyncReadyCallback)dev_states_scan_cb, dev_states);
  }
}
