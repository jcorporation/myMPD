---
title: Pin protection
---

The myMPD settings can be protected with a pin.

To set a pin stop mympd, run `mympd -p` and start myMPD again.

After restart all settings are protected with the entered pin. A lock symbol is displayed for all buttons whose action requires authentication if no valid session is established.

You can goto the main menu and login to create a session, press `L` or simply take an action that is protected (e.g. saving the settings).

The session is valid until restart of myMPD, closing the browser, refreshing the site or you logout.

The [API documentation](../060-references/api/methods.md) shows whether a method is protected or not.
