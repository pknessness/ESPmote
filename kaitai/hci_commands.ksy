meta:
  id: hci_commands
  file-extension: hci_commands
  endian: le
  bit-endian: le
  imports:
    - hci_constants

types:
  link_control:
    params:
      - id: ocf
        type: u2
    seq:
      - id: data
        size-eos: true
        type:
          switch-on: ocf
          cases:
            (hci_constants::link_control_commands::inquiry.as<u2>): inquiry
            (hci_constants::link_control_commands::inquiry_cancel.as<u2>): inquiry_cancel
            (hci_constants::link_control_commands::remote_name_request.as<u2>): remote_name_request
            (hci_constants::link_control_commands::create_connection.as<u2>): create_connection
            _: hci_constants::error_data
            #there are more, I just didn't add them all. its a lot
        
  policy:
    params:
      - id: ocf
        type: u2
    seq:
      - id: data
        size-eos: true        
        type:
          switch-on: ocf
          cases:
            #(policy_commands::change_local_name.as<u2>): name
            #(policy_commands::write_class_of_device.as<u2>): write_class_of_device
            #(policy_commands::write_scan_enable.as<u2>): write_scan_enable
            _: hci_constants::error_data
            #there are more, I just didn't add them all. its a lot
        
  host_controller_baseband:
    params:
      - id: ocf
        type: u2
    seq:
      - id: data
        size-eos: true
        type:
          switch-on: ocf
          cases:
            (hci_constants::host_controller_baseband_commands::change_local_name.as<u2>): name
            (hci_constants::host_controller_baseband_commands::write_class_of_device.as<u2>): write_class_of_device
            (hci_constants::host_controller_baseband_commands::write_scan_enable.as<u2>): write_scan_enable
            _: hci_constants::error_data
            #there are more, I just didn't add them all. its a lot

  name:
    seq:
      - id: name
        type: str
        size-eos: true
        encoding: UTF-8

  #commands
  write_class_of_device:
    seq:
      - id: class_of_device
        type: hci_constants::device_class
  
  write_scan_enable:
    seq:
      - id: status
        type: u1
        
  
  inquiry:
    seq:
      - id: lap
        size: 3
      - id: inquiry_length
        type: u1
      - id: max_responses
        type: u1
  
  inquiry_cancel:
    seq:
      - id: status
        type: u1
        
  remote_name_request:
    seq:
    - id: bd_addr
      type: hci_constants::bd_addr_t
    - id: page_scan_repetition_mode
      type: u1
    - id: reserved
      type: u1
    - id: clock_offset
      type: hci_constants::clock_offset
      
  create_connection:
    seq:
    - id: bd_addr
      type: hci_constants::bd_addr_t
    - id: packet_type
      type: u2
    - id: page_scan_repetition_mode
      type: u1
    - id: reserved
      type: u1
    - id: clock_offset
      type: hci_constants::clock_offset
    - id: allow_role_switch
      type: u1