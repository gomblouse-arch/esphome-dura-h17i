# Dura-H17i → Home Assistant BLE Bridge

**Unofficial community project** — ESPHome-firmware die de Duratech Dura-H17i warmtepomp via BLE koppelt aan Home Assistant. Het DURACLOUD MQTT-over-BLE protocol is reverse-engineered voor interoperabiliteit.

> **Disclaimer:** Dit project is niet gelieerd aan, goedgekeurd of ondersteund door Duratech B.V. Gebruik op eigen risico. Sturing van verwarming/koeling kan veiligheids- en comfortimplicaties hebben. Test grondig voordat je automatiseringen inschakelt.

## Licentie

MIT — zie [LICENSE](LICENSE).

---

## Benodigdheden

| Item | Aanbeveling |
|------|-------------|
| **ESP32** | ESP32-WROOM-32 DevKit (~€8) — **moet BLE hebben** |
| **USB-kabel** | data-kabel (niet alleen laden) |
| **Home Assistant** | Met **ESPHome** add-on |
| **Afstand** | ESP32 binnen ~5–10 m van de warmtepomp |

> **Niet geschikt:** ESP8266, ESP32-S2 (geen BLE), ESP32-H2 (beperkt).

Aanbevolen boards: `ESP32-DevKitC`, `LOLIN D32`, `M5Stack Atom Lite`.

---

## Snel starten

1. Kopieer `esphome/secrets.yaml.example` → `esphome/secrets.yaml` en vul WiFi, OTA én **`heatpump_mac`** in (DURACLOUD-app of nRF Connect).
2. Importeer `esphome/dura-h17i.yaml` in ESPHome (zelfde map als `dura_parser.h`).
3. Flash via USB, daarna OTA.

Uitgebreid stappenplan: [FLASHEREN.md](FLASHEREN.md).

---

## Climate-kaart in Home Assistant

Home Assistant **core** ondersteunt sinds 2025 **geen** template-climate meer. Twee opties:

**A) Losse entiteiten** (aanbevolen) — dashboard met temperatuur, switch, modus, setpoint van device *Dura-H17i*.

**B) Eén climate-kaart** — installeer [hass-template-climate](https://github.com/jcwillox/hass-template-climate) via HACS, kopieer `homeassistant/packages/dura_h17i_climate.yaml` naar HA `config/packages/`, pas entity IDs aan indien nodig, herstart HA.

---

## Testen

1. **Sluit DURACLOUD-app** op je telefoon (BLE = 1 verbinding tegelijk).
2. Druk **BLE herverbinden** in HA.
3. Controleer sensoren: inlaattemperatuur, vermogen, modus.
4. Test aan/uit, setpoint, unit-modus, pooltype, circulatiepomp.
5. Vergelijk kritieke wijzigingen met de DURACLOUD-app.

---

## Entiteiten

| Entiteit | Functie |
|----------|---------|
| `switch.warmtepomp` | Aan/uit |
| `select.modus` | Geluidsmodus: Stille / Standaard / Boost / Slim |
| `select.unit_modus` | Verwarmen / Koelen / Automatisch |
| `select.pooltype` | Zwembad / Spa |
| `select.circulatiepomp` | UIT / AAN / Op verzoek / Interval |
| `number.gewenste_temperatuur` | Setpoint |
| `number.interval_tijd` | Interval circulatiepomp (min, optimistic) |
| `number.controle_tijd` | Controle-tijd (min, optimistic) |
| `number.onetouch_helderheid` | OneTouch helderheid (1–10, range onzeker) |
| `sensor.inlaattemperatuur` | Poolwater (`hiWaterTemperature`) |
| `sensor.vermogen` | Live vermogen (W) |
| `binary_sensor.e03_geen_stroming` | E03 storing |
| `climate.zwembad_warmtepomp` | Climate-kaart (optioneel package) |

Entity IDs krijgen een prefix van je ESPHome device-naam (bijv. `dura_h17i_…`).

---

## Bekende beperkingen

| Onderwerp | Status |
|-----------|--------|
| **BLE exclusief** | App **of** bridge, niet tegelijk |
| **Unit / pool / pomp selects** | Schrijven werkt; teruglezen van pomp nog beperkt (optimistic UI) |
| **Kinderslot** | Niet geïmplementeerd |
| **Firmware-updates Duratech** | Protocol kan wijzigen — meld issues met logs |
| **OneTouch helderheid** | Range 1–10 is een gok (weinig sniff-data) |

---

## BLE-commando's (referentie)

| Actie | Hex (payload) | Bron |
|-------|---------------|------|
| Connect | `020a0100` | sniff |
| Subscribe dashboard | `042202080100` | sniff |
| Aan | `041a08080710051a02080100` | sniff |
| Uit | `041a08080610051a02080100` | sniff |
| Standaard | `041a08080710061a02280100` | sniff |
| Boost | `041a08080710061a02280200` | sniff |
| Slim | `041a08080710061a02280300` | sniff |
| Stille | `041a08080610061a02280100` | sniff |
| Unit Verwarmen | `0a1a08080310011a02280100` (×2) | sniff8 + live |
| Unit Koelen | `0b1a08080310011a02280100` + setpoint | sniff8 + live |
| Unit Automatisch | `0b1a08080310011a02280200` | sniff8 |
| Pool Zwembad / Spa | enum1 `…2801` + setpoint + group3 preamble | sniff8/10 |
| Pump Op verzoek | `…2802` | sniff8 |
| Pump Interval | `…2803` | sniff8 |
| Pump AAN | enum1 `…2801` **2×** (300 ms) | sniff25–27 |
| Pump UIT | enum1 `…2801` **1×** | sniff25–27 |
| Interval / controle / OneTouch | settings group3 int id 6/7/8 | sniff8 |

Volledige analyse: [SNIFF_LOG.md](SNIFF_LOG.md). PCAP-analyse: `python tools/analyze_pcap.py captures/`.

---

## Problemen?

| Probleem | Oplossing |
|----------|-----------|
| Geen BLE-verbinding | DURACLOUD-app sluiten; ESP dichter bij pomp; pomp 30 s uit/aan bij status **133** |
| Verbinding valt weg | Normaal bij veel H17i-units; bridge herverbindt automatisch |
| Alleen vermogen, geen temp | Nieuwste `dura_parser.h` + YAML, opnieuw OTA |
| Modus springt terug | Firmware met strikte parser `08 06 12 02 28` (niet pump-status `08 05`) |
| Compile error | ESPHome add-on updaten; `dura_parser.h` in dezelfde map als YAML |
| Verkeerde entity IDs | Pas climate package aan naar werkelijke namen in HA |

Logs: ESPHome dashboard → **LOGS** op het device.

---

## Bijdragen

Issues en pull requests welkom. Zie [CONTRIBUTING.md](CONTRIBUTING.md).

Deel je ervaring op het [Home Assistant forum](https://community.home-assistant.io/c/projects/21) — tag het project en vermeld ESP32-board + firmwareversie warmtepomp.

---

## Structuur

```
esphome/
  dura-h17i.yaml      # ESPHome firmware
  dura_parser.h       # BLE notify parser (C++)
  secrets.yaml.example
homeassistant/packages/
  dura_h17i_climate.yaml
captures/             # lokale .pcapng (niet in git)
tools/analyze_pcap.py
SNIFF_LOG.md
FLASHEREN.md
```
