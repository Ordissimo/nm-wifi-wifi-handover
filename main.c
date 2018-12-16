#include <stdio.h>
#include <glib.h>
#include <NetworkManager.h>

void show_connection(NMConnection* connection) {
  NMSettingConnection *s_con;
  guint64 timestamp;
  char timestamp_real_str[64];
  const char *val1, *val2, *val3, *val4, *val5;

  s_con = nm_connection_get_setting_connection(connection);
  if (s_con) {
    timestamp = nm_setting_connection_get_timestamp(s_con);
    strftime(timestamp_real_str, sizeof(timestamp_real_str), "%c", localtime ((time_t *) &timestamp));

    val1 = nm_setting_connection_get_id (s_con);
		val2 = nm_setting_connection_get_uuid (s_con);
		val3 = nm_setting_connection_get_connection_type (s_con);
		val4 = nm_connection_get_path (connection);
		val5 = timestamp ? timestamp_real_str : "never";

		printf ("%-25s | %s | %-15s | %-43s | %s\n", val1, val2, val3, val4, val5);
  }
}

void show_device(NMDevice *device) {
  const char *ip_iface = nm_device_get_ip_iface(device);
  const char *iface = nm_device_get_iface(device);
  printf("ip_iface: %s\n", ip_iface);
  printf("iface: %s\n\n", iface);
}


GPtrArray *nm_device_wifi_get_available_aps(NMDeviceWifi* device) {
  const GPtrArray *connections = nm_device_get_available_connections((NMDevice *)device);
  const GPtrArray *aps = nm_device_wifi_get_access_points(device);
  GPtrArray *filtered_aps = g_ptr_array_new();
  for (int i = 0; i < aps->len; i++) {
    NMAccessPoint *ap = aps->pdata[i];
    GPtrArray *matched_conns = nm_access_point_filter_connections(ap, connections);
    if (matched_conns->len > 0) {
      g_ptr_array_add(filtered_aps, ap);
    }
  }

  return filtered_aps;
}

int main(int argc, char const* argv[])
{
  // init client
  NMClient *client;
  GError *error = NULL;
  if (!(client = nm_client_new(NULL, &error))) {
    g_print("Error: Could not connect to NetworkManager: %s.", error->message);
    g_error_free(error);
    return EXIT_FAILURE;
  }
  
  // show strength for each access points for each devices
  const GPtrArray* devices = nm_client_get_devices(client);
  for (int i = 0; i < devices->len; i++) {
    NMDevice *device = devices->pdata[i];
    NMDeviceType type = nm_device_get_device_type(device);
    if (type != NM_DEVICE_TYPE_WIFI) {
      continue;
    }
    
    show_device(device);
    const GPtrArray *aps = nm_device_wifi_get_available_aps((NMDeviceWifi *)device);
    for (int i = 0; i < aps->len; i++) {
      NMAccessPoint *ap = aps->pdata[i];
      GBytes *ssid = nm_access_point_get_ssid(ap);
      guint8 strength = nm_access_point_get_strength(ap);
      printf("%s: %u\n", (char *)g_bytes_get_data(ssid, NULL), strength);
    }

    g_ptr_array_free(aps, TRUE);
  }

  // free resources
  g_object_unref(client);
  return EXIT_SUCCESS;
}
