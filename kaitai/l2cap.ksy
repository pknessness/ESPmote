meta:
  id: l2cap
  file-extension: l2cap
  endian: le
  bit-endian: le
  imports:
    - hci_constants

types:
  packet:
    seq:
      - id: pdu_length
        type: u2
      - id: channel_id
        type: u2
      - id: data
        size: pdu_length
        type:
          switch-on: channel_id
          cases:
            0x0002: data_g
            0x0001: data_c
            _: data_b
  data_g:
    seq:
      - id: pdu_length
        type: u2
      - id: channel_id
        contents: [0x02,0x00]
      - id: data
        size: pdu_length #do wierd repetition shit for the psm
        
  # variable_length_psm:
  #   seq:
  #     - id
  
  data_b:
    seq:
      - id: data
        size-eos: true
        
  data_c:
    seq:
      - id: code
        type: u1
        enum: l2cap_command_codes
      - id: identifier
        type: u1
      - id: data_length
        type: u2
      - id: data
        size: data_length
        type: 
          switch-on: code
          cases:
            'l2cap_command_codes::command_reject_rsp': command_reject_rsp
            'l2cap_command_codes::connection_req': connection_req
            'l2cap_command_codes::connection_rsp': connection_rsp
            'l2cap_command_codes::configuration_req': configuration_req
            'l2cap_command_codes::configuration_rsp': configuration_rsp
            'l2cap_command_codes::disconnection_req': disconnection_req
            'l2cap_command_codes::disconnection_rsp': disconnection_rsp
            'l2cap_command_codes::echo_req': echo_req
            'l2cap_command_codes::echo_rsp': echo_rsp
            'l2cap_command_codes::information_req': information_req
            'l2cap_command_codes::information_rsp': information_rsp
            'l2cap_command_codes::connection_parameter_update_req': connection_parameter_update_req
            'l2cap_command_codes::connection_parameter_update_rsp': connection_parameter_update_rsp
            'l2cap_command_codes::le_credit_based_connection_req': le_credit_based_connection_req
            'l2cap_command_codes::le_credit_based_connection_rsp': le_credit_based_connection_rsp
            'l2cap_command_codes::flow_control_credit_ind': flow_control_credit_ind
            'l2cap_command_codes::credit_based_connection_req': credit_based_connection_req
            'l2cap_command_codes::credit_based_connection_rsp': credit_based_connection_rsp
            'l2cap_command_codes::credit_based_reconfigure_req': credit_based_reconfigure_req
            'l2cap_command_codes::credit_based_reconfigure_rsp': credit_based_reconfigure_rsp
            _: hci_constants::error_data

  # l2cap_control_field: #standard
  #   seq:
  #     - id: type
  #       type: b1
      
  #     #i frame
  #     - id: tx_seq
  #       type: b6
  #     - id: r
  #       type: b1
  #     - id: req_seq
      
  #     #s frame
  command_reject_rsp:
    seq:
      - id: reason
        type: u2
      - id: data
        size-eos: true

  connection_req:
    seq:
      - id: psm
        type: u2
      - id: source_cid
        type: u2

  connection_rsp:
    seq:
      - id: destination_cid
        type: u2
      - id: source_cid
        type: u2
      - id: result
        type: u2
      - id: status
        type: u2

  configuration_req:
    seq:
      - id: destination_cid
        type: u2
      - id: flags
        type: flags
      - id: config_options
        size-eos: true

  configuration_rsp:
    seq:
      - id: source_cid
        type: u2
      - id: flags
        type: flags
      - id: result
        type: u2
      - id: config_options
        size-eos: true

  flags:
    seq:
      - id: c
        type: b1
      - id: rfu
        type: b15

  disconnection_req:
    seq:
      - id: destination_cid
        type: u2
      - id: source_cid
        type: u2

  disconnection_rsp:
    seq:
      - id: destination_cid
        type: u2
      - id: source_cid
        type: u2

  echo_req:
    seq:
      - id: data
        size-eos: true

  echo_rsp:
    seq:
      - id: data
        size-eos: true

  information_req:
    seq:
      - id: info_type
        type: u2

  information_rsp:
    seq:
      - id: info_type
        type: u2
      - id: result
        type: u2
      - id: data
        size-eos: true

  connection_parameter_update_req:
    seq:
      - id: interval_min
        type: u2
      - id: interval_max
        type: u2
      - id: slave_latency
        type: u2
      - id: timeout_multiplier
        type: u2

  connection_parameter_update_rsp:
    seq:
      - id: result
        type: u2

  le_credit_based_connection_req:
    seq:
      - id: le_psm
        type: u2
      - id: source_cid
        type: u2
      - id: mtu
        type: u2
      - id: mps
        type: u2
      - id: initial_credits
        type: u2

  le_credit_based_connection_rsp:
    seq:
      - id: destination_cid
        type: u2
      - id: mtu
        type: u2
      - id: mps
        type: u2
      - id: initial_credits
        type: u2
      - id: result
        type: u2

  flow_control_credit_ind:
    seq:
      - id: cid
        type: u2
      - id: credits
        type: u2

  credit_based_connection_req:
    seq:
      - id: psm
        type: u2
      - id: source_cid
        type: u2
      - id: mtu
        type: u2
      - id: mps
        type: u2
      - id: initial_credits
        type: u2

  credit_based_connection_rsp:
    seq:
      - id: destination_cid
        type: u2
      - id: mtu
        type: u2
      - id: mps
        type: u2
      - id: initial_credits
        type: u2
      - id: result
        type: u2

  credit_based_reconfigure_req:
    seq:
      - id: mtu
        type: u2
      - id: mps
        type: u2
      - id: cids
        type: u2
        repeat: eos

  credit_based_reconfigure_rsp:
    seq:
      - id: result
        type: u2

enums:
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