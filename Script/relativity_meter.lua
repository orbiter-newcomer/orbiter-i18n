-- relativity_meter.lua
-- Relativistic time dilation meter (Terminal MFD script, uses only standard API)
--
-- Usage: in a Terminal MFD press INP and enter:  run('relativity_meter')
--
-- It measures the REAL simulation time acceleration under the hood:
--     measured rate = d(sim time) / d(system time)
-- With the player warp set to 1x this rate equals gamma * gravity factor,
-- i.e. it shows the relativistic time acceleration ("underwater" rate).
-- The velocity Lorentz factor gamma is computed from the focus vessel speed.
--
-- Stop: press DEL on the terminal / switch MFD.

local c = 299792458.0
local note = oapi.create_annotation()
note:set_pos(0.02, 0.90, 0.5, 1.0)
note:set_size(1.2)
note:set_colour({r=1, g=1, b=0.9})
note:set_text("TIME DILATION: waiting for focus vessel...")

print("TIME DILATION meter started. Output shown on screen (top-left).")
print("Set time acceleration to 1x for a clean relativistic measurement.")
print("stop: DEL terminal / switch MFD")

local prev_sim, prev_sys = nil, nil
local rate = 1.0
local cnt = 0

while true do
    local v = vessel.get_focusinterface()
    if v ~= nil then
        local simT = oapi.get_simtime()
        local sysT = oapi.get_systime()
        if prev_sim ~= nil and prev_sys ~= nil then
            local dsys = sysT - prev_sys
            if dsys > 0.2 then
                local ds = simT - prev_sim
                rate = ds / dsys
                prev_sim, prev_sys = simT, sysT
                cnt = cnt + 1
            end
        else
            prev_sim, prev_sys = simT, sysT
        end

        if cnt > 0 then
            local name = v:get_name()
            local vv = v:get_globalvel()
            local sp = math.sqrt(vv.x*vv.x + vv.y*vv.y + vv.z*vv.z)
            local gamma = 1.0
            if sp < c then
                gamma = 1.0/math.sqrt(1.0 - (sp/c)*(sp/c))
            end
            local tacc = oapi.get_tacc()
            note:set_text(string.format(
                "RELATIVITY TIME DILATION [%s]\n" ..
                "measured time rate : %.6f x   (d_sim/d_real)\n" ..
                "internal warp      : %.6f x\n" ..
                "velocity           : %.5e m/s\n" ..
                "velocity gamma     : %.9f\n" ..
                "rate/gamma         : %.6f   (gravity factor if warp=1x)",
                name, rate, tacc, sp, gamma, rate/math.max(gamma,1e-12)))
        end
    end
    proc.skip()
end