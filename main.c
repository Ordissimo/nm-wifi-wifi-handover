#include "./dev_states.h"
#include <NetworkManager.h>
#include <glib.h>
#include <stdio.h>

DevStates dev_states;

int main(int argc, char const *argv[]) {
  GMainLoop *loop;
  loop = g_main_loop_new(NULL, FALSE);

  // init client
  NMClient *client;
  GError *error = NULL;
  if (!(client = nm_client_new(NULL, &error))) {
    g_print("Error: Could not connect to NetworkManager: %s.", error->message);
    g_error_free(error);
    return EXIT_FAILURE;
  }

  // init dev states
  dev_states.mdev = NULL;
  dev_states.devs = g_ptr_array_new();
  dev_states.client = client;

  const GPtrArray *devices = nm_client_get_devices(client);
  for (int i = 0; i < devices->len; i++) {
    NMDevice *device = devices->pdata[i];
    NMDeviceType type = nm_device_get_device_type(device);
    if (type == NM_DEVICE_TYPE_WIFI) {
      g_ptr_array_add(dev_states.devs, device);
    }
  }

  dev_states_request_scan(&dev_states);
  g_main_loop_run(loop);

  // free resources
  g_ptr_array_unref(dev_states.devs);
  g_object_unref(dev_states.client);
  return EXIT_SUCCESS;
}
