-- relativity_meter.lua
-- 相对论加速度计（Terminal MFD 用）
--
-- 用法：在 Terminal MFD 中按 INP 输入：
--     run('relativity_meter')
--
-- 功能：持续测量"焦点飞船"的惯性系加速度（区间平均），每 0.5 秒更新一次
--       屏幕左上角的叠加文字（不刷屏）。不进行任何操作，你自己机动飞船。
--
-- 停止：删除终端（DEL 按钮）或切换 MFD 模式即可。
--
-- 读数说明：
--   |a|    区间平均加速度大小（惯性系，m/s^2）
--   along  沿速度方向分量（纵向；正=加速，负=减速）
--   |v|    当前速率
--   gamma  洛伦兹因子（0.6c 时约 1.25）

local c = 299792458.0
local interval = 0.5        -- 更新间隔（模拟时间，秒）

local note = oapi.create_annotation()
note:set_pos(0.02, 0.9, 0.5, 1.0)
note:set_size(1.2)
note:set_colour({r=1, g=1, b=0.9})
note:set_text("ACCELEROMETER: waiting for focus vessel...")

print("ACCELEROMETER started. Output shown on screen (top-left).")
print("stop: DEL terminal / switch MFD")

local t_ref, v_ref = nil, nil
local last_update = -1e9

while true do
    local v = vessel.get_focusinterface()
    if v ~= nil then
        local t = oapi.get_simtime()
        local vv = v:get_globalvel()
        local name = v:get_name()

        if v_ref == nil then
            t_ref, v_ref = t, vv
            note:set_text("ACCELEROMETER [" .. name .. "]: waiting...")
        elseif (t - last_update) >= interval then
            local dt = t - t_ref
            if dt > 1e-9 then
                local ax = (vv.x - v_ref.x)/dt
                local ay = (vv.y - v_ref.y)/dt
                local az = (vv.z - v_ref.z)/dt
                local a = math.sqrt(ax*ax + ay*ay + az*az)
                local sp = math.sqrt(vv.x*vv.x + vv.y*vv.y + vv.z*vv.z)
                local along = 0.0
                if sp > 1e-9 then
                    along = (ax*vv.x + ay*vv.y + az*vv.z)/sp
                end
                local gamma = 1.0
                if sp < c then
                    gamma = 1.0/math.sqrt(1.0 - (sp/c)*(sp/c))
                end
                note:set_text(string.format("[%s]\n|a| = %.6f m/s2\nalong = %.6f\n|v| = %.6e m/s\ngamma = %.5f",
                    name, a, along, sp, gamma))
            end
            t_ref, v_ref = t, vv
            last_update = t
        end
    end
    proc.skip()
end
