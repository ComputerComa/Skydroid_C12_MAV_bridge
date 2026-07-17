-- Reports whether the onboard-computer MAVLink component is present and active.
-- This script is informational only and does not prevent arming.

local mavlink_msgs = require("MAVLink/mavlink_msgs")

local COMPANION_SYSID = 1
local COMPANION_COMPID = 191
local MAV_STATE_ACTIVE = 4

local HEARTBEAT_TIMEOUT_MS = 3000
local UPDATE_PERIOD_MS = 500

local MAV_SEVERITY_INFO = 6
local MAV_SEVERITY_WARNING = 4

local HEARTBEAT_ID = mavlink_msgs.get_msgid("HEARTBEAT")
local message_map = {
    [HEARTBEAT_ID] = "HEARTBEAT"
}

-- Arguments are the receive queue depth and number of registered message IDs.
mavlink:init(10, 1)
mavlink:register_rx_msgid(HEARTBEAT_ID)

local last_heartbeat_ms = nil
local last_system_status = nil
local previous_state = nil

local function receive_heartbeats()
    while true do
        local raw_message = mavlink:receive_chan()
        if raw_message == nil then
            return
        end

        local message = mavlink_msgs.decode(raw_message, message_map)
        if message ~= nil
            and message.msgid == HEARTBEAT_ID
            and message.sysid == COMPANION_SYSID
            and message.compid == COMPANION_COMPID then
            last_heartbeat_ms = millis()
            last_system_status = message.system_status
        end
    end
end

local function update()
    receive_heartbeats()

    local now = millis()
    local heartbeat_current =
        last_heartbeat_ms ~= nil
        and (now - last_heartbeat_ms) <= HEARTBEAT_TIMEOUT_MS

    local current_state
    if not heartbeat_current then
        current_state = "missing"
    elseif last_system_status ~= MAV_STATE_ACTIVE then
        current_state = "not_ready"
    else
        current_state = "ready"
    end

    if current_state ~= previous_state then
        if current_state == "ready" then
            gcs:send_text(MAV_SEVERITY_INFO, "Companion computer ready")
        elseif current_state == "not_ready" then
            gcs:send_text(MAV_SEVERITY_WARNING,
                          "Companion computer not ready")
        else
            gcs:send_text(MAV_SEVERITY_WARNING,
                          "Companion computer not present")
        end

        previous_state = current_state
    end

    return update, UPDATE_PERIOD_MS
end

return update()
