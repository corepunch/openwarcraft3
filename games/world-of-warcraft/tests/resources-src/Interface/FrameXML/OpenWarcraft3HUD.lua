local elapsed = 0

function ow3_update(msec)
    elapsed = elapsed + msec
end

function ow3_draw()
    local p = ow3.player()
    local inv = ow3.stat(ow3.STAT_INVENTORY_FIRST)

    ow3.draw_image('Interface\\Test\\LuaPanel.blp', 0.100, 0.200, 0.300, 0.050)
    if inv > 0 then
        ow3.draw_image_index(inv, 0.140, 0.260, 0.040, 0.040)
    end
    ow3.draw_color(0.200, 0.320, 0.120, 0.020, 10, 20, 30, 240)
    ow3.draw_text(p.name .. ':' .. p.health .. ':' .. elapsed, 0.210, 0.350, 0.400, 0.030, 13, 255, 220, 120, 255, 'center')
    ow3.draw_minimap(0.700, 0.080, 0.120, 0.120)
    ow3.command('wow_lua_test ' .. p.level .. ' ' .. elapsed)
end
