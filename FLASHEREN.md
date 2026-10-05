# ESP32 flashen — stappenplan

## Bestanden

| Bestand | Doel |
|---------|------|
| `esphome/dura-h17i.yaml` | ESPHome firmware |
| `esphome/dura_parser.h` | BLE parser (zelfde map als YAML) |
| `esphome/secrets.yaml` | WiFi + OTA (lokaal, niet in git) |
| `homeassistant/packages/dura_h17i_climate.yaml` | Optionele climate-kaart |

---

## Stap A — Packages activeren (optioneel, climate-kaart)

In `configuration.yaml`:

```yaml
homeassistant:
  packages: !include_dir_named packages
```

Kopieer `homeassistant/packages/dura_h17i_climate.yaml` naar HA `config/packages/` en herstart HA.

---

## Stap B — ESPHome-bestanden op Home Assistant

1. Open **Studio Code Server** (add-on).
2. Map: `config/esphome/`
3. Kopieer:
   - `dura-h17i.yaml`
   - `dura_parser.h`
4. Maak `config/esphome/secrets.yaml`:

```yaml
wifi_ssid: "JOUW-WIFI-NAAM"
wifi_password: "JOUW-WIFI-WACHTWOORD"
api_encryption_key: "PLAK-HIER-32-TEKENS"
ota_password: "kies-een-sterk-wachtwoord"
heatpump_mac: "AA:BB:CC:DD:EE:FF"
```

5. Zet **`heatpump_mac`** in `secrets.yaml` op de MAC van **jouw** warmtepomp (DURACLOUD / nRF Connect).

> WiFi = **2,4 GHz** waar de ESP32 bij de warmtepomp staat.

---

## Stap C — Device in ESPHome

1. **Instellingen → Add-ons → ESPHome Device Builder → Open Web UI**
2. **+ New device** → importeer `dura-h17i.yaml` uit `config/esphome/`
3. Genereer **API encryption key** bij eerste compile → zelfde key in `secrets.yaml`.

---

## Stap D — Eerste flash (USB)

1. ESP32 via **data-USB** op je PC.
2. Sluit **DURACLOUD-app** (BLE = 1 verbinding).
3. ESPHome: **Install** → **Plug into this computer** (of esptool handmatig).
4. Kies COM-poort, wacht tot **Done**.

Daarna ESP32 bij de warmtepomp (5V USB-voeding).

---

## Stap E — Test in Home Assistant

1. Device **Dura-H17i** moet online zijn onder ESPHome.
2. Entiteiten: temperatuur, `switch.warmtepomp`, selects, numbers.
3. **BLE herverbinden** als er geen data komt.
4. Test aan/uit, setpoint, unit-modus, pooltype, circulatiepomp.

Updates: kopieer gewijzigde YAML + `dura_parser.h`, **Install** (OTA).

---

## Problemen

| Symptoom | Actie |
|----------|--------|
| USB niet zichtbaar | CH340-driver; andere data-kabel |
| Geen BLE | App sluiten; ESP dichter bij warmtepomp |
| Compile error `dura_parser.h` | Zelfde map als YAML op HA |
| Geen climate-entity | Package + `homeassistant.packages` + herstart |
