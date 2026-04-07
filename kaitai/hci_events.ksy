meta:
  id: hci_events
  file-extension: hci_events
  endian: le
  imports:
    - hci_constants

types:
  #events
  inquiry_complete:
    seq:
      - id: status
        type: u1
        enum: event_status

  inquiry_result:
    seq:
      - id: num_responses
        type: u1
      - id: responses_addr
        type: hci_constants::bd_addr_t
        repeat: expr
        repeat-expr: num_responses
      - id: page_scan_repetition_mode
        type: u1
        repeat: expr
        repeat-expr: num_responses
      - id: reserved
        type: u2
        repeat: expr
        repeat-expr: num_responses      
      - id: class_of_device
        type: hci_constants::device_class
        repeat: expr
        repeat-expr: num_responses      
      - id: clock_offset
        type: hci_constants::clock_offset
        repeat: expr
        repeat-expr: num_responses      

  connection_complete:
    seq:
      - id: status
        type: u1
        enum: event_status
      - id: connection_handle
        type: u2
      - id: bd_addr
        type: hci_constants::bd_addr_t
      - id: link_type
        type: u1
      - id: encryption_enabled
        type: u1

  connection_request:
    seq:
      - id: bd_addr
        type: hci_constants::bd_addr_t
      - id: class_of_device
        type: hci_constants::device_class
      - id: link_type
        type: u1

  disconnection_complete:
    seq:
      - id: status
        enum: event_status
        type: u1
      - id: connection_handle
        type: u2
      - id: reason
        type: u1

  authentication_complete:
    seq:
      - id: status
        enum: event_status
        type: u1
      - id: connection_handle
        type: u2

  remote_name_request_complete:
    seq:
      - id: status
        enum: event_status
        type: u1
      - id: bd_addr
        type: hci_constants::bd_addr_t
      - id: remote_name
        type: str
        size: 248
        encoding: ASCII

  encryption_change:
    seq:
      - id: status
        enum: event_status
        type: u1
      - id: connection_handle
        type: u2
      - id: encryption_enabled
        type: u1

  change_connection_link_key_complete:
    seq:
      - id: status
        enum: event_status
        type: u1
      - id: connection_handle
        type: u2

  master_link_key_complete:
    seq:
      - id: status
        enum: event_status
        type: u1
      - id: connection_handle
        type: u2

  read_remote_supported_features_complete:
    seq:
      - id: status
        enum: event_status
        type: u1
      - id: connection_handle
        type: u2
      - id: lmp_features
        type: u8

  read_remote_version_information_complete:
    seq:
      - id: status
        enum: event_status
        type: u1
      - id: connection_handle
        type: u2
      - id: lmp_version
        type: u1
      - id: manufacturer_name
        type: u2
      - id: lmp_subversion
        type: u2

  qos_setup_complete:
    seq:
      - id: status
        enum: event_status
        type: u1
      - id: connection_handle
        type: u2
      - id: flags
        type: u1
      - id: service_type
        type: u1
      - id: token_rate
        type: u4
      - id: peak_bandwidth
        type: u4
      - id: latency
        type: u4
      - id: delay_variation
        type: u4

  command_complete:
    seq:
      - id: num_hci_command_packets
        type: u1
      - id: command_opcode
        type: hci_constants::cmd_op_code
      - id: return_parameters
        size-eos: true

  command_status:
    seq:
      - id: status
        enum: event_status
        type: u1
      - id: num_hci_command_packets
        type: u1
      - id: command_opcode
        type: hci_constants::cmd_op_code

  hardware_error:
    seq:
      - id: hardware_code
        type: u1

  flush_occurred:
    seq:
      - id: connection_handle
        type: u2

  role_change:
    seq:
      - id: status
        enum: event_status
        type: u1
      - id: bd_addr
        type: hci_constants::bd_addr_t
      - id: new_role
        type: u1

  number_of_completed_packets:
    seq:
      - id: num_handles
        type: u1
      - id: connection_handles
        type: u2
        repeat: expr
        repeat-expr: num_handles

  mode_change:
    seq:
      - id: status
        enum: event_status
        type: u1
      - id: connection_handle
        type: u2
      - id: current_mode
        type: u1
      - id: interval
        type: u2

  return_link_keys:
    seq:
      - id: num_keys
        type: u1
      - id: link_keys
        type: hci_constants::bd_addr_t
        repeat: expr
        repeat-expr: num_keys

  pin_code_request:
    seq:
      - id: bd_addr
        type: hci_constants::bd_addr_t

  link_key_request:
    seq:
      - id: bd_addr
        type: hci_constants::bd_addr_t

  link_key_notification:
    seq:
      - id: bd_addr
        type: hci_constants::bd_addr_t
      - id: link_key
        size: 16

  loopback_command:
    seq:
      - id: hci_command_packet
        size-eos: true

  data_buffer_overflow:
    seq:
      - id: link_type
        type: u1

  max_slots_change:
    seq:
      - id: connection_handle
        type: u2
      - id: lmp_max_slots
        type: u1

  read_clock_offset_complete:
    seq:
      - id: status
        enum: event_status
        type: u1
      - id: connection_handle
        type: u2
      - id: clock_offset
        type: hci_constants::clock_offset

  connection_packet_type_changed:
    seq:
      - id: status
        enum: event_status
        type: u1
      - id: connection_handle
        type: u2
      - id: packet_type
        type: u2

  qos_violation:
    seq:
      - id: connection_handle
        type: u2

  page_scan_mode_change:
    seq:
      - id: bd_addr
        type: hci_constants::bd_addr_t
      - id: page_scan_mode
        type: u1

  page_scan_repetition_mode_change:
    seq:
      - id: bd_addr
        type: hci_constants::bd_addr_t
      - id: page_scan_repetition_mode
        type: u1

  flow_specification_complete:
    seq:
      - id: status
        enum: event_status
        type: u1
      - id: connection_handle
        type: u2
      - id: flags
        type: u1
      - id: flow_direction
        type: u1
      - id: service_type
        type: u1
      - id: token_rate
        type: u4
      - id: token_bucket_size
        type: u4
      - id: peak_bandwidth
        type: u4
      - id: access_latency
        type: u4

  inquiry_result_with_rssi:
    seq:
      - id: num_responses
        type: u1
      - id: responses
        type: hci_constants::bd_addr_t
        repeat: expr
        repeat-expr: num_responses

  read_remote_extended_features_complete:
    seq:
      - id: status
        enum: event_status
        type: u1
      - id: connection_handle
        type: u2
      - id: page_number
        type: u1
      - id: max_page_number
        type: u1
      - id: extended_lmp_features
        type: u8

  synchronous_connection_complete:
    seq:
      - id: status
        enum: event_status
        type: u1
      - id: connection_handle
        type: u2
      - id: bd_addr
        type: hci_constants::bd_addr_t
      - id: link_type
        type: u1
      - id: transmission_interval
        type: u1
      - id: retransmission_window
        type: u1
      - id: rx_packet_length
        type: u2
      - id: tx_packet_length
        type: u2
      - id: air_mode
        type: u1

  synchronous_connection_changed:
    seq:
      - id: status
        enum: event_status
        type: u1
      - id: connection_handle
        type: u2
      - id: transmission_interval
        type: u1
      - id: retransmission_window
        type: u1
      - id: rx_packet_length
        type: u2
      - id: tx_packet_length
        type: u2
        

enums:
  hci_event_codes:
    0x01: inquiry_complete
    0x02: inquiry_result
    0x03: connection_complete
    0x04: connection_request
    0x05: disconnection_complete
    0x06: authentication_complete
    0x07: remote_name_request_complete
    0x08: encryption_change
    0x09: change_connection_link_key_complete
    0x0a: master_link_key_complete
    0x0b: read_remote_supported_features_complete
    0x0c: read_remote_version_information_complete
    0x0d: qos_setup_complete
    0x0e: command_complete
    0x0f: command_status
    0x10: hardware_error
    0x11: flush_occurred
    0x12: role_change
    0x13: number_of_completed_packets
    0x14: mode_change
    0x15: return_link_keys
    0x16: pin_code_request
    0x17: link_key_request
    0x18: link_key_notification
    0x19: loopback_command
    0x1a: data_buffer_overflow
    0x1b: max_slots_change
    0x1c: read_clock_offset_complete
    0x1d: connection_packet_type_changed
    0x1e: qos_violation
    0x1f: page_scan_mode_change
    0x20: page_scan_repetition_mode_change
    0x21: flow_specification_complete
    0x22: inquiry_result_with_rssi
    0x23: read_remote_extended_features_complete
    0x2c: synchronous_connection_complete
    0x2d: synchronous_connection_changed
    
  event_status:
    0x00: success
    0x01: unknown_hci_command
    0x02: unknown_connection_identifier
    0x03: hardware_failure
    0x04: page_timeout
    0x05: authentication_failure
    0x06: pin_or_key_missing
    0x07: memory_capacity_exceeded
    0x08: connection_timeout
    0x09: connection_limit_exceeded
    0x0a: synchronous_connection_limit_to_a_device_exceeded
    0x0b: connection_already_exists
    0x0c: command_disallowed
    0x0d: connection_rejected_due_to_limited_resources
    0x0e: connection_rejected_due_to_security_reasons
    0x0f: connection_rejected_due_to_unacceptable_bd_addr
    0x10: connection_accept_timeout_exceeded
    0x11: unsupported_feature_or_parameter_value
    0x12: invalid_hci_command_parameters
    0x13: remote_user_terminated_connection
    0x14: remote_device_terminated_connection_due_to_low_resources
    0x15: remote_device_terminated_connection_due_to_power_off
    0x16: connection_terminated_by_local_host
    0x17: repeated_attempts
    0x18: pairing_not_allowed
    0x19: unknown_lmp_pdu
    0x1a: unsupported_remote_feature
    0x1b: sco_offset_rejected
    0x1c: sco_interval_rejected
    0x1d: sco_air_mode_rejected
    0x1e: invalid_lmp_parameters_invalid_ll_parameters
    0x1f: unspecified_error
    0x20: unsupported_lmp_parameter_value_unsupported_ll_parameter_value
    0x21: role_change_not_allowed
    0x22: lmp_response_timeout_ll_response_timeout
    0x23: lmp_error_transaction_collision_ll_procedure_collision
    0x24: lmp_pdu_not_allowed
    0x25: encryption_mode_not_acceptable
    0x26: link_key_cannot_be_changed
    0x27: requested_qos_not_supported
    0x28: instant_passed
    0x29: pairing_with_unit_key_not_supported
    0x2a: different_transaction_collision
    0x2b: reserved_for_future_use_0x2b
    0x2c: qos_unacceptable_parameter
    0x2d: qos_rejected
    0x2e: channel_classification_not_supported
    0x2f: insufficient_security
    0x30: parameter_out_of_mandatory_range
    0x31: reserved_for_future_use_0x31
    0x32: role_switch_pending
    0x33: reserved_for_future_use_0x33
    0x34: reserved_slot_violation
    0x35: role_switch_failed
    0x36: extended_inquiry_response_too_large
    0x37: secure_simple_pairing_not_supported_by_host
    0x38: host_busy_pairing
    0x39: connection_rejected_due_to_no_suitable_channel_found
    0x3a: controller_busy
    0x3b: unacceptable_connection_parameters
    0x3c: advertising_timeout
    0x3d: connection_terminated_due_to_mic_failure
    0x3e: connection_failed_to_be_established_synchronization_timeout
    0x3f: previously_used
    0x40: coarse_clock_adjustment_rejected_but_will_try_to_adjust_using_clock_dragging
    0x41: type0_submap_not_defined
    0x42: unknown_advertising_identifier
    0x43: limit_reached
    0x44: operation_cancelled_by_host
    0x45: packet_too_long
    0x46: too_late
    0x47: too_early