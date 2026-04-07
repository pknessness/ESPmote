#include "../../btstack/src/btstack.h"

#define CLASS_OF_DEVICE 0x2504
#define VENDOR_ID 0x057E
#define PRODUCT_ID 0x0306


int btstack_main(int argc, const char * argv[]);
int btstack_main(int argc, const char * argv[]){
      // allow to get found by inquiry
  gap_discoverable_control(1);
  // use Limited Discoverable Mode; Peripheral; Keyboard as CoD
  gap_set_class_of_device(CLASS_OF_DEVICE);
  // set local name to be identified - zeroes will be replaced by actual BD ADDR
  gap_set_local_name("Nintendo RVL-CNT-01");
  // allow for role switch in general and sniff mode
  //gap_set_default_link_policy_settings( LM_LINK_POLICY_ENABLE_ROLE_SWITCH | LM_LINK_POLICY_ENABLE_SNIFF_MODE );
  // allow for role switch on outgoing connections - this allow HID Host to become master when we re-connect to it
  //gap_set_allow_role_switch(true);

  // L2CAP
  l2cap_init();

  // SDP Server
  sdp_init();
  memset(hid_service_buffer, 0, sizeof(hid_service_buffer));

  uint8_t hid_virtual_cable = 0;
  uint8_t hid_remote_wake = 1;
  uint8_t hid_reconnect_initiate = 1;
  uint8_t hid_normally_connectable = 1;

  // hid sevice subclass 2540 Keyboard, hid counntry code 33 US, hid virtual cable off, hid reconnect initiate off, hid boot device off 
  hid_create_sdp_record(hid_service_buffer, 0x10001, CLASS_OF_DEVICE, 33, 
    hid_virtual_cable, hid_remote_wake, hid_reconnect_initiate, hid_normally_connectable,
    hid_boot_device, hid_descriptor_keyboard_boot_mode, sizeof(hid_descriptor_keyboard_boot_mode), hid_device_name);
  printf("HID service record size: %u\n", de_get_len( hid_service_buffer));
  sdp_register_service(hid_service_buffer);

  // See https://www.bluetooth.com/specifications/assigned-numbers/company-identifiers if you don't have a USB Vendor ID and need a Bluetooth Vendor ID
  // device info: BlueKitchen GmbH, product 1, version 1
  device_id_create_sdp_record(device_id_sdp_service_buffer, 0x10003, DEVICE_ID_VENDOR_ID_SOURCE_BLUETOOTH, BLUETOOTH_COMPANY_ID_BLUEKITCHEN_GMBH, 1, 1);
  printf("Device ID SDP service record size: %u\n", de_get_len((uint8_t*)device_id_sdp_service_buffer));
  sdp_register_service(device_id_sdp_service_buffer);

  // HID Device
  hid_device_init(hid_boot_device, sizeof(hid_descriptor_keyboard_boot_mode), hid_descriptor_keyboard_boot_mode);

  // register for HCI events
  hci_event_callback_registration.callback = &packet_handler;
  hci_add_event_handler(&hci_event_callback_registration);

  // register for HID events
  hid_device_register_packet_handler(&packet_handler);

#ifdef HAVE_BTSTACK_STDIN
  sscanf_bd_addr(device_addr_string, device_addr);
  btstack_stdin_setup(stdin_process);
#endif  
  // turn on!
  hci_power_control(HCI_POWER_ON);
  return 0;
}

