---
layout: page
permalink: /scripting/usage/battery-indicator
title: Battery level indicator
---

The `BatteryIndicator.lua` script replaces the first icon on your homescreen with a battery level indicator that updates itself every time you click it. It makes use of the ligatures myMPD ships with and relies on your operating system reporting the battery level to a text file in /sys.

- Determine where said file is: most likely `/sys/class/power_supply/[…]/capacity`
- Create a new script and import `BatteryIndicator.lua`
- Copy and paste the path to your `capacity` file into line 1
- Make sure the name you gave to your new script is the same as the last attribute in the last line and save
- Create a timer that runs the script regularly.
