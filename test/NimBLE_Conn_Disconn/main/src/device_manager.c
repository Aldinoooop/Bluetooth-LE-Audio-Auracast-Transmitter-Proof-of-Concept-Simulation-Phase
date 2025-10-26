#include "device_manager.h"
#include "esp_log.h"

#define MAX_CONN 10
static const char *TAG = "DEVICE_MGR";

static int connected_devices[MAX_CONN];
static int conn_count = 0;

void add_connected_device(int conn_handle) {
    if (conn_count < MAX_CONN) {
        connected_devices[conn_count++] = conn_handle;
        // ESP_LOGI(TAG, "Device added, handle=%d", conn_handle);
    }
}

void remove_connected_device(int conn_handle) {
    for (int i = 0; i < conn_count; i++) {
        if (connected_devices[i] == conn_handle) {
            for (int j = i; j < conn_count - 1; j++) {
                connected_devices[j] = connected_devices[j + 1];
            }
            conn_count--;
            // ESP_LOGI(TAG, "Device removed, handle=%d", conn_handle);
            return;
        }
    }
}

void disconnect_device(int conn_handle)
{
    int ret = ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    if (ret == 0) {
        // ESP_LOGI(TAG, "Device disconnected successfully, handle=%d", conn_handle);
    } else {
        // ESP_LOGI(TAG, "Failed to disconnect device handle=%d", conn_handle);
    }
}
