#ifndef DEV_STATES_H
#define DEV_STATES_H
#include <NetworkManager.h>
#include <glib.h>

typedef struct {
  NMDeviceWifi *mdev;
  GPtrArray *devs;
  NMClient *client;
  guint8 base_mdev_ap_strength;
  gulong mdev_handler_id;
} DevStates;

NMAccessPoint *dev_states_find_dev_ap(DevStates *dev_states,
                                      NMDeviceWifi *device, GPtrArray *aps);

void nm_device_wifi_connect_ap(NMDeviceWifi *device, NMClient *client,
                               NMAccessPoint *ap, GAsyncReadyCallback cb,
                               void *user_data);

GPtrArray *nm_device_wifi_get_available_aps(NMDeviceWifi *device);

void dev_states_check_swap(DevStates *dev_states);

void dev_states_scan_cb(NMDeviceWifi *device, GAsyncResult *res,
                        DevStates *dev_states);

void dev_states_request_scan(DevStates *dev_states);
#endif
