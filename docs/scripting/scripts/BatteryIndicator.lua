-- {"order":1,"arguments":[]}
battery = mympd.os_capture("cat /sys/class/power_supply/YourBattery/capacity")
battery = tonumber(battery)

icon = "battery_full"
for i = 6, 0, -1 do
    if battery < math.floor((i * 16.7) + 0.5) then
        icon = "battery_" .. i .. "_bar"
    end
end

battery = battery .. "%"
rc, raw_result = mympd.api("MYMPD_API_HOME_ICON_SAVE", {
    replace = true,
    oldpos = 0,
    name = battery,
    ligature = icon,
    bgcolor = "#ffffff",
    color = "#000000",
    image = "",
    cmd = "execScriptFromOptions",
    options = {"BatteryIndicator"}
})
