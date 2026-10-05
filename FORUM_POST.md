# Home Assistant Community — draft / status

**Category:** Share your Projects!  
**Title:** Duratech Dura-H17i pool heat pump BLE bridge for Home Assistant (ESPHome)

**Status (2026-10-05):** GitHub published. Forum post for this project is **blocked** because account `gomblouse-arch` is silenced (“New user typed too fast”) until a moderator lifts it. The earlier Link Mini topic is still **pending approval**.

Draft body below — paste once the account can post again:

---

Hi all,

I reverse-engineered the DuraCloud MQTT-over-BLE protocol used by the **Duratech Dura-H17i** pool heat pump and built an **ESPHome** bridge so it can be controlled from Home Assistant (without keeping the DuraCloud app connected).

### What it does
- Power on/off
- Setpoint, unit mode (heat / cool / auto)
- Sound mode (Silent / Standard / Boost / Smart)
- Pool type (pool / spa) and circulation pump modes
- Live sensors: inlet water temperature, power, E03 flow fault
- Optional climate card via HACS `hass-template-climate`

### Hardware
- ESP32 with BLE within range of the heat pump
- Same GATT family as other Duratech Link / DuraCloud devices

### Repo
https://github.com/gomblouse-arch/esphome-dura-h17i

MIT licensed. Not affiliated with Duratech/Fluidra. Feedback welcome — especially if your unit firmware behaves differently.

Related: Link Mini / Vision Spectra lights bridge — https://github.com/gomblouse-arch/esphome-duratech-link-mini
