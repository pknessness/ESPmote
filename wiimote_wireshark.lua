-- Simple Wireshark Lua dissector for a custom protocol
local my_proto = Proto("myproto", "My Custom Protocol")

function my_proto.dissector(buffer, pinfo, tree)
pinfo.cols.protocol = my_proto.name
local length = buffer:len()

-- Create a display tree
local subtree = tree:add(my_proto, buffer(), "My Custom Protocol Data")

-- Add fields to the tree
subtree:add(buffer(0,1), "Field 1: " .. buffer(0,1):uint())
subtree:add(buffer(1,2), "Field 2: " .. buffer(1,2):uint())
end
