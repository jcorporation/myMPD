---
layout: page
permalink: /configuration/pin-protection
title: Pin protection
---

The myMPD settings can be protected with a pin. Setting a pin is only supported if myMPD is compiled with enabled ssl.

To set a pin run `mympd -p` and restart myMPD.

After restart all settings are protected with the entered pin.

A lock symbol is displayed for all buttons whose action requires authentication if no valid session is established.

You can goto the main menu and login to create a session. The session is valid until restart of myMPD, closing the browser, refreshing the site or you logout.
