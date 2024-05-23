local function conv_map(map, f, level)
  f:write('{ // level ',level,'\n')
  for l,layer in ipairs(map.layers) do
    f:write('  { // layer ',l-1,'\n')
    for i,spr in ipairs(layer.data) do
      local y = (i-1) // map.width
      local x = (i-1) % map.width
      if x == 0 then f:write('    {') end
      f:write(string.format("%3d", math.max(spr-1,0)))
      if x == map.width-1 then
        f:write('},\n')
      else
        f:write(',')
      end
    end
    f:write('  },\n')
  end
  f:write('},')
end

local f <close> = io.open('../maps.h', 'w')
f:write('{\n')
conv_map(require 'l1', f, 1)
conv_map(require 'l2', f, 2)
conv_map(require 'l3', f, 3)
conv_map(require 'l4', f, 4)
f:write('};\n')
