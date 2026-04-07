meta:
  id: hci
  file-extension: hex
  endian: le
  bit-endian: le
  imports:
    - hci_events
    - hci_commands
    - l2cap

seq:
  - id: packet
    type: hci_wrap
    repeat: eos

types:
  hci_wrap:
    seq:
    - id: txrx
      type: u1
      enum: tx_rx
      doc: 1 is tx, 2 is rx
    - id: packet
      type: hci_packet
  hci_packet:
    seq:
    - id: h4_type
      type: u1
      enum: h4_types
    - id: data
      type: 
        switch-on: h4_type
        cases:
          'h4_types::command': cmd_packet
          'h4_types::async_data': async_packet
          'h4_types::sync_data': sync_packet
          'h4_types::event': event_packet
  
  cmd_packet:
    seq:
      - id: op_code
        type: hci_constants::cmd_op_code
      - id: cmd_length
        type: u1
      - id: cmd_data
        size: cmd_length
        type:
          switch-on: op_code.ogf
          cases:
            1: hci_commands::link_control(op_code.ocf)
            2: hci_commands::policy(op_code.ocf)
            3: hci_commands::host_controller_baseband(op_code.ocf.as<u2>)
            _: hci_constants::error_data
        if: cmd_length > 0
      

  async_packet:
    seq:
      - id: header
        type: acl_header
      - id: acl_length
        type: u2
      - id: l2cap_pa
        size: acl_length
        type: l2cap::packet #assume l2cap

  sync_packet:
    seq:
      - id: header
        type: sco_header
      - id: sco_length
        type: u1
      - id: sco_data
        size: sco_length

  event_packet:
    seq:
      - id: event_code
        type: u1
        enum: hci_events::hci_event_codes
      - id: event_parameters
        type: u1
      - id: evt_data
        size: event_parameters
        type:
          switch-on: event_code
          cases:
            'hci_events::hci_event_codes::inquiry_complete': hci_events::inquiry_complete
            'hci_events::hci_event_codes::inquiry_result': hci_events::inquiry_result
            'hci_events::hci_event_codes::connection_complete': hci_events::connection_complete
            'hci_events::hci_event_codes::connection_request': hci_events::connection_request
            'hci_events::hci_event_codes::disconnection_complete': hci_events::disconnection_complete
            'hci_events::hci_event_codes::authentication_complete': hci_events::authentication_complete
            'hci_events::hci_event_codes::remote_name_request_complete': hci_events::remote_name_request_complete
            'hci_events::hci_event_codes::encryption_change': hci_events::encryption_change
            'hci_events::hci_event_codes::change_connection_link_key_complete': hci_events::change_connection_link_key_complete
            'hci_events::hci_event_codes::master_link_key_complete': hci_events::master_link_key_complete
            'hci_events::hci_event_codes::read_remote_supported_features_complete': hci_events::read_remote_supported_features_complete
            'hci_events::hci_event_codes::read_remote_version_information_complete': hci_events::read_remote_version_information_complete
            'hci_events::hci_event_codes::qos_setup_complete': hci_events::qos_setup_complete
            'hci_events::hci_event_codes::command_complete': hci_events::command_complete
            'hci_events::hci_event_codes::command_status': hci_events::command_status
            'hci_events::hci_event_codes::hardware_error': hci_events::hardware_error
            'hci_events::hci_event_codes::flush_occurred': hci_events::flush_occurred
            'hci_events::hci_event_codes::role_change': hci_events::role_change
            'hci_events::hci_event_codes::number_of_completed_packets': hci_events::number_of_completed_packets
            'hci_events::hci_event_codes::mode_change': hci_events::mode_change
            'hci_events::hci_event_codes::return_link_keys': hci_events::return_link_keys
            'hci_events::hci_event_codes::pin_code_request': hci_events::pin_code_request
            'hci_events::hci_event_codes::link_key_request': hci_events::link_key_request
            'hci_events::hci_event_codes::link_key_notification': hci_events::link_key_notification
            'hci_events::hci_event_codes::loopback_command': hci_events::loopback_command
            'hci_events::hci_event_codes::data_buffer_overflow': hci_events::data_buffer_overflow
            'hci_events::hci_event_codes::max_slots_change': hci_events::max_slots_change
            'hci_events::hci_event_codes::read_clock_offset_complete': hci_events::read_clock_offset_complete
            'hci_events::hci_event_codes::connection_packet_type_changed': hci_events::connection_packet_type_changed
            'hci_events::hci_event_codes::qos_violation': hci_events::qos_violation
            'hci_events::hci_event_codes::page_scan_mode_change': hci_events::page_scan_mode_change
            'hci_events::hci_event_codes::page_scan_repetition_mode_change': hci_events::page_scan_repetition_mode_change
            'hci_events::hci_event_codes::flow_specification_complete': hci_events::flow_specification_complete
            'hci_events::hci_event_codes::inquiry_result_with_rssi': hci_events::inquiry_result_with_rssi
            'hci_events::hci_event_codes::read_remote_extended_features_complete': hci_events::read_remote_extended_features_complete
            'hci_events::hci_event_codes::synchronous_connection_complete': hci_events::synchronous_connection_complete
            'hci_events::hci_event_codes::synchronous_connection_changed': hci_events::synchronous_connection_changed
            _: hci_constants::error_data
        if: event_parameters > 0
        
  acl_header:
    seq:
      - id: raw2
        type: u2
    instances:
      handle:
        value: (raw2 & 0xFFF0) >> 4
      pb_flag:
        value: (raw2 & 0x3)
      bc_flag:
        value: (raw2 & 0xC) >> 2
  sco_header:
    seq:
      - id: raw2
        type: u2
    instances:
      handle:
        value: (raw2 & 0xFFF0) >> 4
      packet_status_flag:
        value: (raw2 & 0x3)
      rfu:
        value: (raw2 & 0xC) >> 2
  
  
  
enums:
  h4_types:
    1: command
    2: async_data
    3: sync_data
    4: event
    9: extended_command
  tx_rx:
    1: from_wii #tx on the wii side
    2: to_wii #rx on the wii side
  l2cap_command_codes:
    0x01: command_reject_rsp
    0x02: connection_req
    0x03: connection_rsp
    0x04: configuration_req
    0x05: configuration_rsp
    0x06: disconnection_req
    0x07: disconnection_rsp
    0x08: echo_req
    0x09: echo_rsp
    0x0a: information_req
    0x0b: information_rsp
    0x12: connection_parameter_update_req
    0x13: connection_parameter_update_rsp
    0x14: le_credit_based_connection_req
    0x15: le_credit_based_connection_rsp
    0x16: flow_control_credit_ind
    0x17: credit_based_connection_req
    0x18: credit_based_connection_rsp
    0x19: credit_based_reconfigure_req
    0x1a: credit_based_reconfigure_rsp