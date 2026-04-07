meta:
  id: hci_constants
  file-extension: hci_constants
  endian: le
  bit-endian: le
  
types:
  device_class:
    seq:
      # - id: class
      #   size: 3
      - id: format_type
        type: b2
      - id: minor_device_class
        type: b6
      - id: major_device_class
        type: b5
        enum: major_device_class
      - id: reserved_for_service_classes
        type: b11
  error_data:
    seq:
      - id: err
        size: 255
  cmd_op_code:
    seq:
      - id: ocf
        type: b10
      - id: ogf
        type: b6
    instances:
      ocf_link_control:
        value: ocf
        enum: link_control_commands
        if: ogf == 1
      ocf_policy:
        value: ocf
        enum: policy_commands
        if: ogf == 2
      ocf_host_controller_baseband:
        value: ocf
        enum: host_controller_baseband_commands
        if: ogf == 3
      
  clock_offset:
    seq:
      - id: offset16_2
        type: b15
      - id: valid
        type: b1
  bd_addr_t:
    seq:
      - id: addr
        size: 6
        
enums:
  major_device_class:
    0x00: miscellaneous
    0x01: computer
    0x02: phone
    0x03: lan_network_access_point
    0x04: audio_video
    0x05: peripheral
    0x06: imaging
    0x07: wearable
    0x08: toy
    0x09: health
    0x1F: uncategorized
    
  link_control_commands:
    0x0001: inquiry
    0x0002: inquiry_cancel
    0x0003: periodic_inquiry_mode
    0x0004: exit_periodic_inquiry_mode
    0x0005: create_connection
    0x0006: disconnect
    0x0007: add_sco_connection
    0x0009: accept_connection_request
    0x000a: reject_connection_request
    0x000b: link_key_request_reply
    0x000c: link_key_request_negative_reply
    0x000d: pin_code_request_reply
    0x000e: pin_code_request_negative_reply
    0x000f: change_connection_packet_type
    0x0011: authentication_requested
    0x0013: set_connection_encryption
    0x0015: change_connection_link_key
    0x0017: master_link_key
    0x0019: remote_name_request
    0x001b: read_remote_supported_features
    0x001d: read_remote_version_information
    0x001f: read_clock_offset
    
  policy_commands:
    0x0001: hold_mode
    0x0003: sniff_mode
    0x0004: exit_sniff_mode
    0x0005: park_mode
    0x0006: exit_park_mode
    0x0007: qos_setup
    0x0009: role_discovery
    0x000b: switch_role
    0x000c: read_link_policy_settings
    0x000d: write_link_policy_settings
    
  host_controller_baseband_commands:
    0x0001: set_event_mask
    0x0003: reset
    0x0005: set_event_filter
    0x0008: flush
    0x0009: read_pin_type
    0x000a: write_pin_type
    0x000b: create_new_unit_key
    0x000d: read_stored_link_key
    0x0011: write_stored_link_key
    0x0012: delete_stored_link_key
    0x0013: change_local_name
    0x0014: read_local_name
    0x0015: read_connection_accept_timeout
    0x0016: write_connection_accept_timeout
    0x0017: read_page_timeout
    0x0018: write_page_timeout
    0x0019: read_scan_enable
    0x001a: write_scan_enable
    0x001b: read_page_scan_activity
    0x001c: write_page_scan_activity
    0x001d: read_inquiry_scan_activity
    0x001e: write_inquiry_scan_activity
    0x001f: read_authentication_enable
    0x0020: write_authentication_enable
    0x0021: read_encryption_mode
    0x0022: write_encryption_mode
    0x0023: read_class_of_device
    0x0024: write_class_of_device
    0x0025: read_voice_setting
    0x0026: write_voice_setting
    0x0027: read_automatic_flush_timeout
    0x0028: write_automatic_flush_timeout
    0x0029: read_num_broadcast_retransmissions
    0x002a: write_num_broadcast_retransmissions
    0x002b: read_hold_mode_activity
    0x002c: write_hold_mode_activity
    0x002d: read_transmit_power_level
    0x002e: read_sco_flow_control_enable
    0x002f: write_sco_flow_control_enable
    0x0031: set_host_controller_to_host_flow_control
    0x0033: host_buffer_size
    0x0035: host_number_of_completed_packets
    0x0036: read_link_supervision_timeout
    0x0037: write_link_supervision_timeout
    0x0038: read_number_of_supported_iac
    0x0039: read_current_iac_lap
    0x003a: write_current_iac_lap
    0x003b: read_page_scan_period_mode
    0x003c: write_page_scan_period_mode
    0x003d: read_page_scan_mode
    0x003e: write_page_scan_mode