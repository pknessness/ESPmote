local wii_hid_proto = Proto("wii", "Wii Protocol")
local wiimote_hid_proto = Proto("wiimote", "Wiimote Protocol")


function wii_hid_proto.dissector(tvb, pinfo, tree)
    local length = tvb:len()

    -- need min 11 bytes
    if length < 11 then return end

    local report_id = tvb(10,1):uint()

    -- if
    if tvb(9,1):uint() == 0xa2 then
        pinfo.cols.protocol = "Wii Send"
        local subtree = tree:add(wii_hid_proto, tvb, "Packet")
        if report_id == 0x11 then
            if length == 12 then
                pinfo.cols.info = string.format("W: Player LEDs: [%s %s %s %s]", led1 and "1" or ".", led2 and "2" or ".", led3 and "3" or ".", led4 and "4" or ".")
                local ll = tvb(11, 1):uint()
                local rumble = (ll & 0x01) ~= 0
                local led1 = (ll & 0x10) ~= 0
                local led2 = (ll & 0x20) ~= 0
                local led3 = (ll & 0x40) ~= 0
                local led4 = (ll & 0x80) ~= 0

                subtree:add(tvb(11, 1), string.format("LEDs: [%s %s %s %s]", led1 and "1" or ".", led2 and "2" or ".", led3 and "3" or ".", led4 and "4" or "."))
                if rumble then
                    subtree:add(tvb(11, 1), "Rumble")
                end
            end

        elseif report_id == 0x12 then
            if length == 13 then
                local tt = tvb(11, 1):uint()
                local mm = tvb(12, 1):uint()
                local continuous = (tt & 0x04) ~= 0
                local rumble = (tt & 0x01) ~= 0

                pinfo.cols.info = string.format("W: Change Reporting Mode 0x%02x", mm)

                subtree:add(tvb(11, 1), string.format("TT: 0x%02x (Continuous: %s)", tt, continuous and "Yes" or "No"))
                subtree:add(tvb(12, 1), string.format("MM: 0x%02x (Mode: %s)", mm, mm >= 0x30 and string.format("0x%02x", mm) or "Invalid"))
                if rumble then
                    subtree:add(tvb(11, 1), "Rumble")
                end
            end

        elseif report_id == 0x13 then
            pinfo.cols.info = "W: IR Camera Enable"
            if length == 12 then
                local ee = tvb(11, 1):uint()
                local enabled = (ee & 0x04) ~= 0
                local rumble = (ee & 0x01) ~= 0

                subtree:add(tvb(11, 1), string.format("IR Camera: %s", enabled and "Enabled" or "Disabled"))
                if rumble then
                    subtree:add(tvb(11, 1), "Rumble")
                end
            end

        elseif report_id == 0x14 then
            pinfo.cols.info = "W: Speaker Enable"
            if length == 12 then
                local ee = tvb(11, 1):uint()
                local enabled = (ee & 0x04) ~= 0
                local rumble = (ee & 0x01) ~= 0

                subtree:add(tvb(11, 1), string.format("Speaker: %s", enabled and "Enabled" or "Disabled"))
                if rumble then
                    subtree:add(tvb(11, 1), "Rumble")
                end
            end

        elseif report_id == 0x15 then
            pinfo.cols.info = "W: Status Info Request"
            if length == 12 then
                local rr = tvb(11, 1):uint()
                local rumble = (rr & 0x01) ~= 0

                if rumble then
                    subtree:add(tvb(11, 1), "Rumble")
                end
            end

        elseif report_id == 0x16 then
            if length == 32 then
                local mm = tvb(11, 1):uint()
                local addr = tvb(12, 3):uint()
                local size = tvb(15, 1):uint()
                local rumble = (mm & 0x01) ~= 0

                if ((mm & 0x4) ~= 0) then
                    pinfo.cols.info = string.format("W: Write Register Request at 0x%06x", addr)
                elseif (mm & 0x4) == 0 then
                    pinfo.cols.info = string.format("W: Write Memory Request at 0x%06x", addr)
                end
                subtree:add(tvb(11, 1), string.format("MM: 0x%02x", mm))
                subtree:add(tvb(12, 3), string.format("Address: 0x%06x", addr))
                subtree:add(tvb(15, 1), string.format("Size: %d bytes", size))
                subtree:add(tvb(16, size), string.format("Data: %s", tvb(16,size):bytes():tohex()))
                if rumble then
                    subtree:add(tvb(11, 1), "Rumble")
                end
            end

        elseif report_id == 0x17 then
            if length == 17 then
                local mm = tvb(11, 1):uint()
                local addr = tvb(12, 3):uint()
                local size = tvb(15, 2):uint()
                local rumble = (mm & 0x01) ~= 0

                if(mm & 0x4) then
                    pinfo.cols.info = string.format("W: Read Register Request at 0x%06x", addr)
                elseif (mm & 0x4) == 0 then
                    pinfo.cols.info = string.format("W: Read Memory Request at 0x%06x", addr)
                end

                subtree:add(tvb(11, 1), string.format("MM: 0x%02x", mm))
                subtree:add(tvb(12, 3), string.format("Address: 0x%06x", addr))
                subtree:add(tvb(15, 2), string.format("Size: %d bytes", size))
                if rumble then
                    subtree:add(tvb(11, 1), "Rumble")
                end
            end

        elseif report_id == 0x18 then
            pinfo.cols.info = "W: Speaker Data"
            if length == 32 then
                local ll = tvb(11, 1):uint()
                local rumble = (ll & 0x01) ~= 0

                subtree:add(tvb(11, 1), string.format("LL: 0x%02x (length shift)", ll))
                subtree:add(tvb(12, 20), "Speaker Data (20 bytes)")
                if rumble then
                    subtree:add(tvb(11, 1), "Rumble")
                end
            end

        elseif report_id == 0x19 then
            pinfo.cols.info = "W: Speaker Mute"
            if length == 12 then
                local ee = tvb(11, 1):uint()
                local muted = (ee & 0x04) ~= 0
                local rumble = (ee & 0x01) ~= 0

                subtree:add(tvb(11, 1), string.format("Mute: %s", muted and "Muted" or "Unmuted"))
                if rumble then
                    subtree:add(tvb(11, 1), "Rumble")
                end
            end

        elseif report_id == 0x1a then
            pinfo.cols.info = "W: IR Camera Enable 2"
            if length == 12 then
                local ee = tvb(11, 1):uint()
                local enabled = (ee & 0x04) ~= 0
                local rumble = (ee & 0x01) ~= 0

                subtree:add(tvb(11, 1), string.format("IR Camera 2: %s", enabled and "Enabled" or "Disabled"))
                if rumble then
                    subtree:add(tvb(11, 1), "Rumble")
                end
            end
        end
    end

--     -- Get the 11th byte (index 10, since Lua is 0-indexed)
--     local report_id = tvb(10,1):uint()
--
--     if report_id >= 0x11 and report_id <= 0x1a then
--         pinfo.cols.protocol = "W: Wiimote"
--         pinfo.cols.info = "W: Wii Remote: Report " .. string.format("0x%02x", report_id)
--
--         local subtree = tree:add(wii_hid_proto, tvb, "Wii Remote HID")
--         subtree:add(tvb(10,1), "Report ID: ", report_id)
--     elseif report_id >= 0x20 and report_id <= 0x3f then
--         pinfo.cols.protocol = "W: Wiimote"
--         pinfo.cols.info = "W: Wii Remote: Report " .. string.format("0x%02x", report_id)
--
--         local subtree = tree:add(wii_hid_proto, tvb, "Wii Remote HID")
--         subtree:add(tvb(10,1), "Report ID: 0x%02x", report_id)
--     end
end


function wiimote_hid_proto.dissector(tvb, pinfo, tree)
    local length = tvb:len()

    -- need min 11 bytes
    if length < 11 then return end

    local report_id = tvb(10,1):uint()

    -- if
    if tvb(9,1):uint() == 0xa1 then
        pinfo.cols.protocol = "Wiimote Send"
        local subtree = tree:add(wiimote_hid_proto, tvb, "Packet")
        if report_id == 0x20 then
            pinfo.cols.info = "W: Status Reply"
            if length == 17 then
                local bb1 = tvb(11, 1):uint()
                local bb2 = tvb(12, 1):uint()
                local lf = tvb(13, 1):uint()
                local vv = tvb(16, 1):uint()

                local battery_low = (lf & 0x01) ~= 0
                local extension = (lf & 0x02) ~= 0
                local speaker_enabled = (lf & 0x04) ~= 0
                local ir_enabled = (lf & 0x08) ~= 0
                local led1 = (lf & 0x10) ~= 0
                local led2 = (lf & 0x20) ~= 0
                local led3 = (lf & 0x40) ~= 0
                local led4 = (lf & 0x80) ~= 0

                subtree:add(tvb(11, 2), string.format("Buttons: 0x%02x%02x", bb1, bb2))
                subtree:add(tvb(13, 1), string.format("Flags: BatteryLow:%s Ext:%s Speaker:%s IR:%s",
                    battery_low and "1" or ".", extension and "1" or ".",
                    speaker_enabled and "1" or ".", ir_enabled and "1" or "."))
                subtree:add(tvb(13, 1), string.format("LEDs: [%s %s %s %s]",
                    led1 and "1" or ".", led2 and "2" or ".", led3 and "3" or ".", led4 and "4" or "."))
                subtree:add(tvb(14, 2), "Reserved: 0x0000")
                subtree:add(tvb(16, 1), string.format("Battery: 0x%02x", vv))
            end

        elseif report_id == 0x21 then
            if length == 32 then
                local bb1 = tvb(11, 1):uint()
                local bb2 = tvb(12, 1):uint()
                local se = tvb(13, 1):uint()
                local aa1 = tvb(14, 1):uint()
                local aa2 = tvb(15, 1):uint()

                local size = (se >> 4) + 1
                local err = se & 0x0F

                if(err ~= 0) then
                    pinfo.cols.info = string.format("W: Rejected Mem/Reg Response 0x%02x%02x", aa1, aa2)
                else
                    pinfo.cols.info = string.format("W: Read Mem/Reg Response 0x%02x%02x", aa1, aa2)
                end

                subtree:add(tvb(11, 2), string.format("Buttons: 0x%02x%02x", bb1, bb2))
                subtree:add(tvb(13, 1), string.format("SE: 0x%02x (Size:%d bytes Error:%d)", se, size, err))
                subtree:add(tvb(14, 2), string.format("Address: 0x%02x%02x", aa1, aa2))
                subtree:add(tvb(16, 16), string.format("Data: %s", tvb(16,size):bytes():tohex()))
            end

        elseif report_id == 0x22 then
            if length == 15 then
                local bb1 = tvb(11, 1):uint()
                local bb2 = tvb(12, 1):uint()
                local rr = tvb(13, 1):uint()
                local ee = tvb(14, 1):uint()

                pinfo.cols.info = string.format("W: Acknowledge Report %s", (ee ~= 0) and "Reject" or "");

                subtree:add(tvb(11, 2), string.format("Buttons: 0x%02x%02x", bb1, bb2))
                subtree:add(tvb(13, 1), string.format("Report ID: 0x%02x", rr))
                subtree:add(tvb(14, 1), string.format("Error: 0x%02x", ee))
            end

        elseif report_id == 0x30 then
            pinfo.cols.info = "W: Core Buttons"
            if length == 13 then
                local bb1 = tvb(11, 1):uint()
                local bb2 = tvb(12, 1):uint()

                subtree:add(tvb(11, 2), string.format("Buttons: 0x%02x%02x", bb1, bb2))
            end

        elseif report_id == 0x31 then
            pinfo.cols.info = "W: Core Buttons and Accelerometer"
            if length == 16 then
                local bb1 = tvb(11, 1):uint()
                local bb2 = tvb(12, 1):uint()
                local ax = tvb(13, 1):uint()
                local ay = tvb(14, 1):uint()
                local az = tvb(15, 1):uint()

                subtree:add(tvb(11, 2), string.format("Buttons: 0x%02x%02x", bb1, bb2))
                subtree:add(tvb(13, 3), string.format("Accel: X:0x%02x Y:0x%02x Z:0x%02x", ax, ay, az))
            end

        elseif report_id == 0x32 then
            pinfo.cols.info = "W: Core Buttons with 8 Extension Bytes"
            if length == 21 then
                local bb1 = tvb(11, 1):uint()
                local bb2 = tvb(12, 1):uint()

                subtree:add(tvb(11, 2), string.format("Buttons: 0x%02x%02x", bb1, bb2))
                subtree:add(tvb(13, 8), "Extension Data (8 bytes)")
            end

        elseif report_id == 0x33 then
            pinfo.cols.info = "W: Core Buttons and Accelerometer with 12 IR Bytes"
            if length == 28 then
                local bb1 = tvb(11, 1):uint()
                local bb2 = tvb(12, 1):uint()
                local ax = tvb(13, 1):uint()
                local ay = tvb(14, 1):uint()
                local az = tvb(15, 1):uint()

                subtree:add(tvb(11, 2), string.format("Buttons: 0x%02x%02x", bb1, bb2))
                subtree:add(tvb(13, 3), string.format("Accel: X:0x%02x Y:0x%02x Z:0x%02x", ax, ay, az))
                subtree:add(tvb(16, 12), "IR Camera Data (12 bytes)")
            end

        elseif report_id == 0x34 then
            pinfo.cols.info = "W: Core Buttons with 19 Extension Bytes"
            if length == 32 then
                local bb1 = tvb(11, 1):uint()
                local bb2 = tvb(12, 1):uint()

                subtree:add(tvb(11, 2), string.format("Buttons: 0x%02x%02x", bb1, bb2))
                subtree:add(tvb(13, 19), "Extension Data (19 bytes)")
            end

        elseif report_id == 0x35 then
            pinfo.cols.info = "W: Core Buttons and Accelerometer with 16 Extension Bytes"
            if length == 32 then
                local bb1 = tvb(11, 1):uint()
                local bb2 = tvb(12, 1):uint()
                local ax = tvb(13, 1):uint()
                local ay = tvb(14, 1):uint()
                local az = tvb(15, 1):uint()

                subtree:add(tvb(11, 2), string.format("Buttons: 0x%02x%02x", bb1, bb2))
                subtree:add(tvb(13, 3), string.format("Accel: X:0x%02x Y:0x%02x Z:0x%02x", ax, ay, az))
                subtree:add(tvb(16, 16), "Extension Data (16 bytes)")
            end

        elseif report_id == 0x36 then
            pinfo.cols.info = "W: Core Buttons with 10 IR Bytes and 9 Extension Bytes"
            if length == 32 then
                local bb1 = tvb(11, 1):uint()
                local bb2 = tvb(12, 1):uint()

                subtree:add(tvb(11, 2), string.format("Buttons: 0x%02x%02x", bb1, bb2))
                subtree:add(tvb(13, 10), "IR Camera Data (10 bytes)")
                subtree:add(tvb(23, 9), "Extension Data (9 bytes)")
            end

        elseif report_id == 0x37 then
            pinfo.cols.info = "W: Core Buttons and Accelerometer with 10 IR Bytes and 6 Extension Bytes"
            if length == 32 then
                local bb1 = tvb(11, 1):uint()
                local bb2 = tvb(12, 1):uint()
                local ax = tvb(13, 1):uint()
                local ay = tvb(14, 1):uint()
                local az = tvb(15, 1):uint()

                subtree:add(tvb(11, 2), string.format("Buttons: 0x%02x%02x", bb1, bb2))
                subtree:add(tvb(13, 3), string.format("Accel: X:0x%02x Y:0x%02x Z:0x%02x", ax, ay, az))
                subtree:add(tvb(16, 10), "IR Camera Data (10 bytes)")
                subtree:add(tvb(26, 6), "Extension Data (6 bytes)")
            end

        elseif report_id == 0x3d then
            pinfo.cols.info = "W: 21 Extension Bytes"
            if length == 32 then
                subtree:add(tvb(11, 21), "Extension Data (21 bytes)")
            end

        elseif report_id == 0x3e then
            pinfo.cols.info = "W: Interleaved Core Buttons and Accelerometer with 36 IR Bytes (Part 1)"
            if length == 32 then
                local bb1 = tvb(11, 1):uint()
                local bb2 = tvb(12, 1):uint()
                local ax = tvb(13, 1):uint()

                subtree:add(tvb(11, 2), string.format("Buttons: 0x%02x%02x", bb1, bb2))
                subtree:add(tvb(13, 1), string.format("Accel X: 0x%02x", ax))
                subtree:add(tvb(14, 18), "IR Camera Data (18 bytes)")
            end

        elseif report_id == 0x3f then
            pinfo.cols.info = "W: Interleaved Core Buttons and Accelerometer with 36 IR Bytes (Part 2)"
            if length == 32 then
                local bb1 = tvb(11, 1):uint()
                local bb2 = tvb(12, 1):uint()
                local ay = tvb(13, 1):uint()

                subtree:add(tvb(11, 2), string.format("Buttons: 0x%02x%02x", bb1, bb2))
                subtree:add(tvb(13, 1), string.format("Accel Y: 0x%02x", ay))
                subtree:add(tvb(14, 18), "IR Camera Data (18 bytes)")
            end
        end
    end
end

-- Register with wtap_encap (link-layer encapsulation)
local wtap_table = DissectorTable.get("wtap_encap")
if wtap_table then
    -- WTAP_ENCAP_BLUETOOTH_H4 = 191, WTAP_ENCAP_BLUETOOTH_H4_WITH_PHDR = 235
    wtap_table:add(191, wii_hid_proto)
    wtap_table:add(235, wii_hid_proto)
    wtap_table:add(191, wiimote_hid_proto)
    wtap_table:add(235, wiimote_hid_proto)
    print("Registered wiimote with wtap_encap")
end

-- Also try as post-dissector as fallback
register_postdissector(wii_hid_proto)
register_postdissector(wiimote_hid_proto)
print("Registered wiimote as post-dissector")
