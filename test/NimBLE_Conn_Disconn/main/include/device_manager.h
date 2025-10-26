#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

void add_connected_device(int conn_handle);
void remove_connected_device(int conn_handle);
void disconnect_device(int conn_handle);
#endif // DEVICE_MANAGER_H
