# Review-log — Dura-H17i warmtepomp (ESP + Home Assistant)

**Datum:** 2026-08-07  
**Scope:** ESPHome-firmware (`dura-h17i.yaml` / `dura_parser.h`), live HA-entities, climate-package, gerelateerde automations  
**Live ESP:** `dura-h17i` — online, config-hash `c2a8c392`, ESPHome op chip **2026.7.3** (addon **2026.7.4**)  
**Methode:** code-review + live entity-states + HA-config lookup  

---

## Samenvatting

| Ernst | Aantal | Kern |
|-------|--------|------|
| **Bug (live bevestigd)** | 1 | `Verwarmt` blijft `on` terwijl warmtepomp `off` is |
| **Fout / risico** | 4 | State-detectie via ASCII-strings, offline-automation dekt BLE-verlies niet, typo in E03-naam, outdated comments |
| **Verbetering** | 10+ | Parser/state-sync, code-opschoning, sensors, alerts, docs |

De bridge werkt voor bediening (BLE connected, temps, setpoint, modus Slim). De grootste inhoudelijke fout zit in **status-interpretatie** (`Verwarmt`), niet in de write-paden.

---

## 1. Live status (snapshot 2026-08-07 ~09:57)

| Entity | State | Opmerking |
|--------|-------|-----------|
| BLE verbonden | `on` | OK |
| BLE bridge switch | `on` | OK |
| Warmtepomp switch | `off` | OK |
| Warmtepomp aan (binary) | `off` | OK |
| **Verwarmt** | **`on`** | **FOUT — inconsistent met power off + vermogen 4 W** |
| Climate | `off` / hvac_action `off` | Consistent met switch (negeert `Verwarmt` terecht via template) |
| Modus | Slim | OK |
| Unit / Pool / Circulatiepomp | Verwarmen / Zwembad / UIT | Optimistic (geen teruglees-sync) |
| Inlaat / Uitlaat / Omgeving | 27.3 / 27.2 / 22.3 °C | OK |
| Setpoint / Gewenste temp | 27.0 | OK |
| Vermogen | 4 W | Idle/standby-niveau |
| E03 | `off` | OK |

History sinds gisteren ~19:50 (na ESP reconnect): `warmtepomp_aan=off` en `verwarmt=on` staan **tegelijk** vast — geen latere correctie.

---

## 2. Bugs & fouten

### B1 — KRITIEK: `Verwarmt` false positive (live)

**Bestand:** `esphome/dura-h17i.yaml` (notify-lambda)  

Detectie gebruikt `dura::contains(data, "hiHeatpumpStateHeating")` (ASCII substring in BLE-notify). Schema-/dashboard-pakketten bevatten vaak **alle** state-namen als tekst, ook als de pomp niet verwarmt.

Huidige guard:

```cpp
if (dura::contains(data, "hiHeatpumpStateHeating")) {
  if (!pwr_known || pwr_on) {
    id(heating_active).publish_state(true);
  }
}
```

Probleem:

1. Packet **zonder** `hiPower` maar **met** schema-string `hiHeatpumpStateHeating` → `pwr_known=false` → `Verwarmt=on`.
2. Latere packet met power=off wist `Verwarmt`, maar een volgend schema-packet zet het weer aan.
3. Live resultaat: power `off`, vermogen ~4 W, `Verwarmt` toch `on`.

**Aanbevolen fix:**

- Alleen `heating_active=true` als `id(heatpump_power).state == true` (globale power-state, niet alleen huidige packet).
- Bij preference: echte enum/state-bytes parsen i.p.v. ASCII `contains`.
- Alternatief: `Verwarmt` afleiden uit vermogen-drempel (bijv. > 200 W) + power on — pragmatisch maar minder precies.

**Impact HA:** climate-kaart gebruikt `Verwarmt` alleen voor `hvac_action` wanneer unit ≠ Koelen en switch aan is — daarom toont climate nu toch `off`. Dashboards/automations die alleen op `Verwarmt` vertrouwen, liegen.

---

### B2 — E03-detectie: typo in string + fragiel

```yaml
dura::contains(data, "hiIdleStateNoFLow")  # hoofdletter L in "FLow"
```

Als de pomp `hiIdleStateNoFlow` (kleine `l`) of een andere spelling stuurt, mist de detectie. Daarnaast: `data.size() < 200` filter is een heuristiek — grote packets met echte E03 kunnen gemist worden; kleine packets met alleen de stringnaam kunnen false positives geven.

**Aanbeveling:** beide spellings matchen; bij voorkeur typed enum i.p.v. substring; push-automation op E03 (nu geen).

---

### B3 — Offline-automation dekt alleen ESP↔HA, niet BLE↔pomp

`automation.warmtepomp_esp_offline` triggert op:

`binary_sensor.…_ble_verbonden` → `unavailable` (2 min)

Dat is correct voor “ESP weg van HA”.  
Als BLE naar de pomp wegvalt blijft de ESP online → sensor gaat naar **`off`**, niet `unavailable` → **geen push**.

**Aanbeveling:** tweede automation of extra trigger: `to: "off"` + `for: 5–10 min` met andere tekst (“BLE-verbinding met warmtepomp verbroken”).

---

### B4 — Outdated comments in firmware (verwarring bij onderhoud)

In `dura-h17i.yaml` staat bij Unit modus nog:

> `Koelen=0 (synth/unknown)`

Live code schrijft Koelen als **enum1 + setpoint** (niet enum0). Zelfde soort verouderde labels elders in comments. README BLE-tabel is al bijgewerkt; YAML-comments nog niet overal.

---

### B5 — Bestandsnaam typo op HA

Live ESPHome-config: `dura-h17i-bleutooth-esp32.yaml` (**bleutooth**). Werkt, maar verwarrend bij backups/docs. Hernoemen breekt niets functioneel als je ESPHome-device meekoppelt, maar is optioneel opschonen.

---

## 3. Verbeteringen (prioriteit)

### P1 — Hoog (functionaliteit / betrouwbaarheid)

| ID | Verbetering | Waarom |
|----|-------------|--------|
| P1.1 | Fix `Verwarmt` (zie B1) | Live liegende status |
| P1.2 | Group3 notify-parser voor Unit / Pool / Circulatiepomp | Selects zijn optimistic; HA kan na app-wijziging of herstart verkeerd tonen |
| P1.3 | BLE-verlies push (zie B3) | Nu stil als pomp-BLE wegvalt |
| P1.4 | E03 push-notification | Storing is probleem-class; geen alert |

### P2 — Medium (codekwaliteit)

| ID | Verbetering | Waarom |
|----|-------------|--------|
| P2.1 | Interval/check/OneTouch writes: gebruik `dura::make_settings_int_command()` | YAML dupliceert ~30 regels C++; foutgevoelig |
| P2.2 | Noise-mode: gebruik `dura::find_noise_mode()` i.p.v. inline loop | Parser heeft al de strikte helper; YAML herhaalt logica |
| P2.3 | Ambient/Outlet: `ESP_LOGI` bij elke notify → spam | INFO-log bij ~1 Hz; liever DEBUG of throttle |
| P2.4 | Sensor float: `filters: - round: 1` | HA toont `27.299999…` |
| P2.5 | Comments/README sync (B4) | Minder kans op verkeerde “fixes” |

### P3 — Laag (nice-to-have)

| ID | Verbetering | Waarom |
|----|-------------|--------|
| P3.1 | Kinderslot (bewust niet gedaan) | Alleen als sniffer-payload bekend is |
| P3.2 | Utility meter / energie-statistiek op `sensor.…_vermogen` | Langetermijn verbruik |
| P3.3 | Climate: `device_class` / availability al OK; optioneel `fan_modes` weglaten (n.v.t.) | — |
| P3.4 | ESPHome OTA naar 2026.7.4 | `update_available: true`; geen feature-blocker |
| P3.5 | Verborgen interne binary `Warmtepomp aan` vs switch | Dubbel; binary mag `internal: true` als alleen switch nodig is |
| P3.6 | HAOS volledige backup naast `backup-mijn-esp` | Dashboard + HACS + push-scripts |

---

## 4. Wat goed werkt

- BLE handshake + reconnect-cycle (45 s delay, geen reconnect-storm)
- Temperatuurparse typed (water=3, ambient/out=2, setpoint=6) — voorkomt SP-as-inlet bug
- Noise-modus strikte match `08 06 12 02 28` — geen snapback via pump `08 05`
- Circulatiepomp AAN×2 / UIT×1 (sniffer25–27)
- Unit Koelen/Verwarmen/Auto + Pool Spa/Zwembad write-paden (live getest eerder)
- Select `publish_state` voorkomt HA UI-snapback
- Climate-package entity IDs komen overeen met live prefix `zwembad_warmtepomp_dura_h17i_*`
- Offline-ESP automation gebruikt `script.push_naar_personen` (centrale push) — goed patroon
- API encryption actief; secrets niet in public repo

---

## 5. Home Assistant — gerelateerd

### Direct Dura-H17i

| Item | Status |
|------|--------|
| Package `packages/dura_h17i_climate.yaml` | Aanwezig, climate OK |
| HACS `climate_template` | Vereist (extern) |
| `automation.warmtepomp_esp_offline` | OK voor ESP-offline; zie B3 |
| Dashboard “Bediening” | Bevat warmtepomp-refs (niet in YAML-backup) |

### Zwembad-automations (vullen / Shelly / flow)

Er zijn ~12+ automations rond **zwembadvullen / kleppen / Shelly** (`zwembad_flow_detectie_*`, timers, etc.). Die sturen de **Dura-H17i niet** aan (geen directe koppeling in search). Buiten scope van deze firmware-review, tenzij je wilt dat E03 of warmtepomp-aan de vulketen blokkeert — dat bestaat nu niet.

---

## 6. Repo vs live

| | Repo (`esphome/`) | Live HA / chip |
|--|-------------------|----------------|
| MAC | Placeholder `AA:BB:CC:DD:EE:FF` | Echte MAC (correct voor publicatie) |
| Config hash | — | `c2a8c392` |
| Backup | `backup-mijn-esp/` (gitignore) | Bevat flash + climate + offline-automation |

Publicatie-repo mag placeholder houden; voor herstel altijd `backup-mijn-esp` gebruiken.

---

## 7. Voorgestelde actievolgorde

1. **Fix B1** (`Verwarmt`) in YAML → OTA  
2. **B3** BLE-offline push toevoegen  
3. **B2 + E03-push**  
4. P2-opschoning (helpers, logs, round) bij volgende OTA-batch  
5. P1.2 group3 state-sync wanneer er tijd is voor sniffer-werk  

---

## 8. Open vragen (niet geblokkeerd)

- Exacte wire-bytes voor actieve heatpump-state (niet alleen ASCII-namen) — nodig voor robuuste `Verwarmt`/`Koelt`
- OneTouch helderheid-range (nog gok 1–10)
- Kinderslot-payload onbekend
- Of Duratech-firmware-updates het BLE-protocol wijzigen

---

*Einde review-log. Geen code gewijzigd in deze pass — alleen analyse.*
