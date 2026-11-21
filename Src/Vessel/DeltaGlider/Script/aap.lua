-- Set up autopilot structures
aap = {}
aap.altid = nil
aap.spdid = nil
aap.bnkid = nil
aap.hdgid = nil
aap.tgtalt = nil
aap.tgtspd = nil
aap.tgtbnk = nil
aap.tgthdg = nil
aap.maxasc = 30
aap.maxdsc = -20

-- Define help page
dg_aap = {file='Html/Script/Stockvessels/DG/aap.chm'}

function setvessel (_v)
    if _v then
        v = _v
        class = _v:get_classname()
        if (class ~= 'DeltaGlider') and (class ~= 'DG-S') then
            term.out('Warning: Autopilot is designed for use with DeltaGlider.')
        end
    end
end

function altitude ()
    alt = v:get_altitude()
    return alt
end

function airspeed ()
    return v:get_airspeed()
end

function slope ()
    as = v:get_airspeedvector(REFFRAME.HORIZON)
    xz = math.sqrt (as.x^2 + as.z^2)
    sl = math.atan2 (as.y, xz)
    return sl
end

--[[ --------------------------------------------------------
altitude autopilot (elevator control)
argument 1: target altitude (m)
note: does not terminate (run as background job and kill
      when done)
--]]

function alt_ap (alt)
    if alt == nil then
        alt = 20e3
    end
    aap.tgtalt = alt
    local dslope0 = 0
    local dslope_rate_f = 0
    -- Tuning (2026-11-21): the original control law used an effective P gain of
    -- 1.0 (elevator rate per slope error) and a D gain of 10 with the raw
    -- numerical derivative of the slope error. The D term amplified the
    -- step-to-step noise of the derivative, which showed up as a high frequency
    -- elevator chatter. Gains are reduced, the derivative is low-pass filtered,
    -- a small altitude deadband and an elevator rate limit were added. The
    -- outer height-error -> target-slope mapping is unchanged.
    local kp_slope = 0.3      -- was 1.0  [1/s]
    local kd_slope = 2.0      -- was 10.0
    local rate_max = 1.0      -- max elevator change rate [1/s] (was unlimited)
    local db_alt   = 2.0      -- altitude deadband [m] (was none)
    while true do
        if aap.tgtalt ~= nil then
            local dt = oapi.get_simstep()
            if dt < 1e-6 then dt = 1e-6 end
            alt = altitude()
            dalt = aap.tgtalt-alt
            tgt_slope = RAD*1e-2 * dalt
	        if tgt_slope > aap.maxasc*RAD then
		        tgt_slope = aap.maxasc*RAD
	        elseif tgt_slope < aap.maxdsc*RAD then
		        tgt_slope = aap.maxdsc*RAD
	        end
            dslope = tgt_slope-slope()
            if dslope0 == 0 then
                dslope_rate = 0
            else
                dslope_rate = (dslope-dslope0)/dt
            end
            dslope0 = dslope
            dslope_rate_f = 0.8*dslope_rate_f + 0.2*dslope_rate   -- low-pass filter
            local delev = 0
            if math.abs(dalt) > db_alt then
                delev = (dslope*kp_slope + dslope_rate_f*kd_slope)*dt
                local dmax = rate_max*dt
                if delev > dmax then delev = dmax elseif delev < -dmax then delev = -dmax end
            end
            elev = v:get_adclevel(AIRCTRL.ELEVATOR)
            elev = elev+delev;
            if elev > 1 then elev = 1 elseif elev < -1 then elev = -1 end
            v:set_adclevel(AIRCTRL.ELEVATOR,elev)
        end
        proc.skip()
    end
end

--[[ --------------------------------------------------------
airspeed autopilot (throttle control)
argument 1: target speed (m/s)
note: does not terminate (run as background job and kill
      when done)
--]]

function spd_ap (spd)
    if spd ~= nil then aap.tgtspd = spd end
    local spd0 = airspeed()
    local acc_f = 0
    -- Tuning (2026-11-21): the original law was
    --   dthrott = (dspd*alpha - acc*beta)*dt,  alpha = beta = 1
    -- where acc is the raw numerical derivative of the airspeed. The integral
    -- gain was too high (1/s: the whole speed error is cancelled within one
    -- second) and the unfiltered derivative fed measurement noise straight into
    -- the throttle. At large fixed steps that saturates into 0/1 PWM, at small
    -- variable steps it turns into high frequency throttle chatter. The gains
    -- are reduced, the acceleration estimate is low-pass filtered, and a
    -- deadband plus a throttle rate limit were added.
    local alpha = 0.15        -- was 1.0 [1/s] speed-error integral gain
    local beta  = 0.2         -- was 1.0 acceleration (D) feedback gain
    local db_vel  = 1.0       -- speed error deadband [m/s] (was none)
    local rate_max = 1.0      -- max throttle change rate [1/s] (was unlimited)
    while true do
        if aap.tgtspd ~= nil then
            local dt = oapi.get_simstep()
            if dt < 1e-6 then dt = 1e-6 end
            spd = airspeed()
            acc = (spd-spd0)/dt
            spd0 = spd
            acc_f = 0.8*acc_f + 0.2*acc              -- low-pass filter
            dspd = aap.tgtspd-spd
            local dthrott = 0
            if math.abs(dspd) > db_vel then
                dthrott = (dspd*alpha - acc_f*beta)*dt
                local dmax = rate_max*dt
                if dthrott > dmax then dthrott = dmax elseif dthrott < -dmax then dthrott = -dmax end
            end
            thrott = v:get_thrustergrouplevel (THGROUP.MAIN) + dthrott
            if thrott < 0 then thrott = 0 elseif thrott > 1 then thrott = 1 end
            v:set_thrustergrouplevel (THGROUP.MAIN, thrott)
        end
		proc.skip()
    end
end

--[[ --------------------------------------------------------
bank autopilot (aileron control)
argument 1: bank angle [deg]
note: does not terminate (run as background job and kill
      when done)
--]]

function bank_ap (bnk)
    aap.tgtbnk = bnk
    local bnk0 = v:get_bank()
    local rate_f = 0
    -- Tuning (2026-11-21): the heading autopilot oscillated and diverged. The
    -- bank loop is the inner loop of the heading->bank cascade; the original
    -- effective law was d(ail)/dt = -0.5*bank_error + 1.5*roll_rate with the raw
    -- numerical roll-rate derivative (noisy). Gains are retuned for a faster,
    -- better damped inner loop, the roll-rate term is low-pass filtered, a bank
    -- deadband and an aileron rate limit are added.
    local kp_bank = 1.6       -- was 0.5 (effective P term -0.5*bank_error) [1/s] (2026-11-21: 1.2 -> 1.6, inner loop faster)
    local kd_bank = 2.6       -- was 1.5 (roll-rate damping) (2026-11-21: 2.0 -> 2.6)
    local db_bank = 0.3*RAD   -- bank deadband [rad] (was none)
    local rate_max = 1.2      -- max aileron change rate [1/s] (was unlimited) (2026-11-21: 1.0 -> 1.2)
    while true do
        local dt = oapi.get_simstep()
        if dt < 1e-6 then dt = 1e-6 end
        bnk = v:get_bank()
        dbnk = bnk-bnk0
        if dbnk < -PI then dbnk = dbnk+2*PI elseif dbnk > PI then dbnk = dbnk-2*PI end -- phase unwrap
        rate = dbnk/dt
        bnk0 = bnk
        rate_f = 0.8*rate_f + 0.2*rate       -- low-pass filter (D-term noise)
        dbnk = aap.tgtbnk*RAD - bnk
        if dbnk < -PI then dbnk = dbnk+2*PI elseif dbnk > PI then dbnk = dbnk-2*PI end -- phase unwrap
        local dail = 0
        if math.abs(dbnk) > db_bank then
            dail = (-dbnk*kp_bank + rate_f*kd_bank)*dt
            local dmax = rate_max*dt
            if dail > dmax then dail = dmax elseif dail < -dmax then dail = -dmax end
        end
        ail = v:get_adclevel(AIRCTRL.AILERON)
        ail = ail+dail
        if ail > 1 then ail = 1 elseif ail < -1 then ail = -1 end
        v:set_adclevel(AIRCTRL.AILERON,ail)
        proc.skip()
    end
end

--[[ --------------------------------------------------------
heading autopilot (aileron+pitch control)
argument 1: heading angle [deg]
note: does not terminate (run as background job and kill
      when done)
--]]

function heading_ap (hdg)
    aap.tgthdg = hdg
    local hdg0 = v:get_yaw()
    local tgtbank = v:get_bank()
    local rate_f = 0
    -- Tuning (2026-11-21): the original law used
    --   d(tgtbank)/dt = -100*yaw_error + 1000*yaw_rate
    -- P = 100/s is far too high for the bank inner loop: a 1 deg heading error
    -- asked for ~1.75 rad/s of bank change, so the cascade oscillated and
    -- diverged (especially with variable steps). Gains are reduced, the
    -- yaw-rate term is low-pass filtered, a heading deadband and a target-bank
    -- rate limit are added.
    local kp_hdg = 6.0        -- was 100 [1/s] (2026-11-21 re-tune: 3 -> 6, faster response)
    local kd_hdg = 20.0       -- was 1000 (2026-11-21 re-tune: 10 -> 20, keep damping ratio)
    local db_hdg = 0.5*RAD    -- heading deadband [rad] (was none)
    local rate_max = 0.8      -- max target bank change rate [rad/s] (was unlimited; 0.5 -> 0.8)
    while true do
        local dt = oapi.get_simstep()
        if dt < 1e-6 then dt = 1e-6 end
        hdg = v:get_yaw()
        dhdg = hdg-hdg0
        if dhdg < -PI then dhdg = dhdg+2*PI elseif dhdg > PI then dhdg = dhdg-2*PI end -- phase unwrap
        rate = dhdg/dt
        hdg0 = hdg
        rate_f = 0.8*rate_f + 0.2*rate       -- low-pass filter (D-term noise)
        dhdg = aap.tgthdg*RAD - hdg
        if dhdg < -PI then dhdg = dhdg+2*PI elseif dhdg > PI then dhdg = dhdg-2*PI end -- phase unwrap
        if math.abs(dhdg) > db_hdg then
            dbank = (-dhdg*kp_hdg + rate_f*kd_hdg)*dt
            local dmax = rate_max*dt
            if dbank > dmax then dbank = dmax elseif dbank < -dmax then dbank = -dmax end
            tgtbank = v:get_bank() + dbank
            if tgtbank > 0.3*PI then tgtbank = 0.3*PI elseif tgtbank < -0.3*PI then tgtbank = -0.3*PI end
            aap.tgtbnk = tgtbank*DEG
        end
        proc.skip()
    end
end

-- -----------------------------------------------------------
-- User level entry functions
-- -----------------------------------------------------------

-- Invoke altitude autopilot as background job

function aap.alt (alt)
    if alt == nil then
        if aap.altid ~= nil then
            term.out('AP: altitude autopilot disabled')
            proc.kill(aap.altid)
            aap.altid = nil
            v:set_adclevel(AIRCTRL.ELEVATOR,0)
        end
    else
        term.out('AP: altitude target '..alt..' m')
        if aap.altid == nil then
            aap.altid = proc.bg(alt_ap,alt) -- launch altitude autopilot
        else
            aap.tgtalt = alt -- autopilot already running: just reset altitude
        end
    end
end

-- Invoke speed autopilot as background job

function aap.spd (spd)
    if spd == nil then
        if aap.spdid ~= nil then
            term.out('AP: airspeed autopilot disabled')
            proc.kill(aap.spdid)
            aap.spdid = nil
        end
    else
        term.out('AP: airspeed target '..spd..' m/s')
        if aap.spdid == nil then
            aap.spdid = proc.bg(spd_ap,spd) -- launch airspeed autopilot
        else
            aap.tgtspd = spd -- autopilot already running: just reset target speed
        end
    end
end

-- Invoke bank autopilot as background job

function aap.bank (bank)
    if bank == nil then
        if aap.bnkid ~= nil then
            term.out('AP: bank autopilot disabled')
            proc.kill(aap.bnkid)
            aap.bnkid = nil
            v:set_adclevel(AIRCTRL.AILERON,0)
        end
    else
        term.out('AP: bank target '..bank..' deg')
        if aap.bnkid == nil then
            aap.bnkid = proc.bg(bank_ap,bank) -- launch bank autopilot
        else
            aap.tgtbnk = bank -- autopilot already running: just reset target angle
        end
    end
end

-- Invoke heading autopilot as background job

function aap.hdg (hdg)
    if hdg == nil then
        if aap.hdgid ~= nil then
            term.out('AP: heading autopilot disabled')
            proc.kill(aap.hdgid)
            aap.bank() -- kill bank ap
            aap.hdgid = nil
        end
    else
        term.out('AP: heading target '..hdg..' deg')
        if aap.hdgid == nil then
            aap.hdgid = proc.bg(heading_ap,hdg) -- launch heading autopilot
            aap.bank(0) -- invoke bank ap
        else
            aap.tgthdg = hdg -- autopilot already running: just reset target angle
        end
    end
end

-- -----------------------------------------------------------
-- Initialisation code

v = {}

setvessel(V)

term.out('DG: Atmospheric autopilot loaded.')
term.out('For help, type: help(dg_aap)')