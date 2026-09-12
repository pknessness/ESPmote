meta:
  id: sdp
  file-extension: sdp
  endian: be
  bit-endian: le
  
seq:
  - id: sdp_length
    type: data_header
  - id: packet
    type: sdp_attribute
    repeat: eos

types:
  sdp_attribute:
    seq:
      - id: attribute_id_fixed
        contents: [0x09]
      - id: attribute_id
        type: u2
        enum: service_attribute_ids
      - id: attribue_value
        type: data_element
  data_header:
    seq:
      - id: size_descriptor
        type: b3
      - id: type_descriptor
        type: b5
        enum: data_element_type
      - id: size_data
        type: 
          switch-on: size_descriptor
          cases:
            5: u1
            6: u2
            7: u4
        if: size_descriptor > 4
    instances:
      data_length:
        value: '(size_descriptor > 4) ? (size_data) : (1 << size_descriptor)'
  data_element:
    seq:
      - id: data_element_header
        type: data_header
        #if: data_element_header.type_descriptor == data_element_type::data_element_sequence
      - id: data
        size: data_element_header.data_length
        type:
          switch-on: data_element_header.type_descriptor
          cases:
            'data_element_type::text_string': name
            'data_element_type::data_element_sequence': data_element_sequence
            'data_element_type::data_element_alternative': data_element_sequence
            'data_element_type::unsigned_integer': uint_type
            'data_element_type::signed_integer': sint_type
            'data_element_type::uuid': uuid_type
            'data_element_type::boolean': bool_type
            'data_element_type::uniform_resource_locator': url_type

    instances:
      ins_attribute_type:
        value: data_element_header.type_descriptor
      # ins_value:
      #   value: data.value
      #   if: data_element_type.type_descriptor == data_element_type::unsigned_integer

  uint_type:
    seq:
      - id: value
        type:
          switch-on: _parent.data_element_header.data_length
          cases:
            1: u1
            2: u2
            4: u4
            8: u8  # or use b8 for raw bytes

  sint_type:
    seq:
      - id: value
        type:
          switch-on: _parent.data_element_header.data_length
          cases:
            1: s1
            2: s2
            4: s4
            8: s8

  uuid_type:
    seq:
      - id: value
        size: _parent.data_element_header.data_length

  bool_type:
    seq:
      - id: value
        type: u1

  url_type:
    seq:
      - id: value
        type: str
        encoding: UTF-8
        size-eos: true
  
  data_element_sequence:
    seq:
      - id: elements
        type: data_element
        repeat: eos
  
  name:
    seq:
      - id: name
        type: str
        encoding: UTF-8
        size-eos: true
enums:
  data_element_type:
    0: nil
    1: unsigned_integer
    2: signed_integer
    3: uuid
    4: text_string
    5: boolean
    6: data_element_sequence
    7: data_element_alternative
    8: uniform_resource_locator
  
  service_attribute_ids:
    0x0000: service_record_handle
    0x0001: service_class_id_list
    0x0002: service_record_state
    0x0003: service_id
    0x0004: protocol_descriptor_list
    0x0005: browse_group_list
    0x0006: language_base_attribute_id_list
    0x0007: service_info_time_to_live
    0x0008: service_availability
    0x0009: bluetooth_profile_descriptor_list
    0x000A: documentation_url
    0x000B: client_executable_url
    0x000C: icon_url
    0x000D: additional_protocol_descriptor_lists
    0x0100: service_name
    0x0101: service_description
    0x0102: provider_name
    0x0301: supported_data_stores
    0x0200: specification_id
    0x0201: vendor_id
    0x0202: product_id
    0x0203: version
    0x0204: primary_record
    0x0205: vendor_id_source
