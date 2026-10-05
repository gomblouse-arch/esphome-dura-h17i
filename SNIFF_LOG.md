# Dura-h17i BLE sniff log

## sniffer19..pcapng -- 2026-08-04

**File:** `captures/sniffer19..pcapng`  
**Size:** 251392 bytes | **Duration:** ~57.3 s | **Frames:** 3527 (nordic_ble)  
**Procedure (intended):** start Koelen + Spa + Pump AAN + Child ON (no sniffer); start capture; tap Verwarmen, Zwembad, Pump UIT, Child OFF

### ATT counts

| Opcode | Name | Count |
|--------|------|------:|
| 0x01 | Error Response | 7 |
| 0x02 | Exchange MTU Request | 1 |
| 0x04 | Find Info Request | 4 |
| 0x05 | Find Info Response | 4 |
| 0x08 | Read By Type Request | 12 |
| 0x09 | Read By Type Response | 5 |
| 0x0a | Read Request | 2 |
| 0x0b | Read Response | 2 |
| 0x10 | Read By Group Request | 4 |
| 0x11 | Read By Group Response | 4 |
| 0x12 | Write Request | 21 |
| 0x13 | Write Response | 18 |
| 0x1b | Handle Value Notification | 44 |
| **Total** | **btatt** | **128** |

Write handle of interest: **0x000f** (14 writes with value). Notify handle: **0x000c** (44).

### Framing note (settings writes)

Phone -> device writes on `0x000f` look like:

`[seq] 1a 08 [proto] 00`

where `[proto]` = `08 <group> 10 <field> 1a 02 <value-tag><value>`  
Example: seq `0a`, proto `080310011a022801` -> full `0a1a08080310011a02280100`.

### Full timeline (meaningful ATT value traffic)

Times are relative seconds from capture start.

| t (s) | Frame | Op | Handle | Hex | Notes |
|------:|------:|----|--------|------|-------|
| 14.471 | 805 | WR | 0x0019 | `07` | CCCD-ish |
| 14.524 | 807 | NTF | 0x0019 | `08` | |
| 14.576 | 812 | WR | 0x0019 | `07` | |
| 14.681 | 815 | NTF | 0x0019 | `08` | |
| 14.996 | 830 | WR | 0x000f | `020a0100` | session/hello |
| 15.153 | 846 | WR | 0x000f | `020a0100` | |
| 15.311 | 862 | WR | 0x000f | `042202080100` | select group 1 |
| 15.521 | 900 | WR | 0x000f | `042202080100` | |
| 15.420+ | 895+ | NTF | 0x000c | (schema blobs) | IO/Settings descriptors |
| 16.57-19.57 | 946-1051 | NTF | 0x000c | `0a32...` IO state | f01=`2208` f05=`0801` |
| 19.931 | 1062 | WR | 0x000f | `052202080300` | select **group 3** (Settings) |
| 20.14-21.04 | 1105-1263 | NTF | 0x000c | schema + settings | **pre-tap Settings** f1263: mode01=`2801` pool02=`2801` pump05=`2801` child09=`0801` |
| **22.504** | **1326** | **WR** | **0x000f** | **`0a1a08080310011a02280100`** | UnitMode field1 -- inner `080310011a022801` (**val=1**, not 0) |
| 25.658 | 1583 | NTF | 0x000c | settings | mode01=`2821` pool02=`2801` pump05=`2801` child09=`0801` |
| **26.074** | **1595** | **WR** | **0x000f** | **`0a1a08080310021a02280100`** | PoolType field2 -- inner `080310021a022801` (**val=1**, not 0) |
| 27.18-33.27 | 1759-2095 | NTF | 0x000c | settings | mode01=`2808` pool02=`2819` pump05=`2801` child09=`0801` |
| 33.219 | 2084 | WR | 0x000f | `042202080100` | select group 1 |
| **34.317** | **2148** | **WR** | **0x000f** | **`041a08080610051a02080100`** | **Pump path?** group **6** field5 bool **1** -- NOT `080310051a022800` |
| 34.27 | 2145 | NTF | 0x000c | IO | f05=`0801` (pre) |
| 35.37+ | 2216+ | NTF | 0x000c | IO | f05=`0824` (changed) |
| 37.782 | 2299 | WR | 0x000f | `052202080300` | select group 3 |
| 38.888 | 2473 | NTF | 0x000c | settings | pump05 still `2801` (unchanged) |
| **40.145** | **2522** | **WR** | **0x000f** | **`0a1a08080310091a02080100`** | ChildLock -- inner `080310091a020801` (**ON=1**, not OFF) |
| **41.037** | **2678** | **WR** | **0x000f** | **`0b1a08080310091a02080100`** | ChildLock retry (seq 0b) same ON |
| **42.508** | **2839** | **WR** | **0x000f** | **`0a1a08080310091a02080100`** | ChildLock retry again ON |
| 43.666 | 3022 | NTF | 0x000c | settings | child09=`0823` (odd; was `0801`) pump05 still `2801` |
| 44.818 | 3087 | WR | 0x000f | `042202080100` | select group 1 |
| 45.87-55.85 | 3152-3479 | NTF | 0x000c | IO | f05=`0824` |

### B-reverse checklist (expected vs found)

| Target | Expected inner | Found? | Actual |
|--------|----------------|:------:|--------|
| Verwarmen (-> Heat / enum0) | `080310011a022800` | **NO** | `080310011a022801` (enum **1**) |
| Zwembad (-> Pool / enum0) | `080310021a022800` | **NO** | `080310021a022801` (enum **1**) |
| Pump UIT (-> Off / enum0) | `080310051a022800` | **NO** | no `08031005` at all |
| Child OFF (-> bool0) | `080310091a020800` | **NO** | `080310091a020801` x3 (bool **1**) |

**Pre-tap Settings state (f1263) matches intended start:** Koelen (`01=2801`), Spa (`02=2801`), pump on-ish (`05=2801`), Child ON (`09=0801`).

### Alternate 0-encodings

Searched all ATT values / all `0x000f` writes for:

| Pattern | Meaning | In writes | In notifies |
|---------|---------|:---------:|:-----------:|
| `1a022800` / `2800` | enum value 0 | **0** | **0** |
| `1a020800` / bool `0800` | bool false | **0** | **0** |
| `1a00` | empty value field | **0** | -- |
| omitted value / different settings frame for 0 | -- | **not seen** | -- |
| `2208` | empty-message style (often enum0 in state) | **0** | **16** (IO state `f01=2208` only) |

**Alternate non-zero path of interest (not a 0-encoding):** Pump tap produced  
`04 1a 08 08 06 10 05 1a 02 08 01 00` -> group **6** + **bool** `0801`, not Settings group **3** + enum `2800`. Settings `pump05` stayed `2801`; only IO `f05` moved `0801`->`0824`.

No write used empty/`2208`/omitted-field form to express 0.

### Verdict

**Not ready** for B-reverse wire confirmation of enum0/bool0.

**Tip for next sniff:** Same start state (Koelen/Spa/Pump AAN/Child ON). After Wireshark starts, confirm UI still shows those four, then one deliberate tap each to **Verwarmen / Zwembad / Pump UIT / Child OFF**. Watch that the app actually leaves the start value (do not re-tap the already-selected option). Expect inners ending `2800` / `0800` (or document whatever alternate 0 form appears). If Pump again goes via group 6, capture that full hex -- it may be the real Off path, but this run only showed bool **1**.


## sniffer20..pcapng -- 2026-08-04

**File:** `captures/sniffer20..pcapng`  
**Size:** 860232 bytes | **Duration:** ~285.2 s | **Frames:** 12212 | **CONNECT_IND:** 3 (14:17:46, 14:18:56, 14:19:23)  
**Observed UI actions (from writes):** Koelen, setpoint 30.5, Spa, setpoint 30.0, Koelen again, noise Boost, noise Slim, **power OFF**, **power ON** — **not** the B-reverse quartet (Verwarmen / Zwembad / Pump UIT / Child OFF).

### ATT counts

| Opcode | Name | Count |
|--------|------|------:|
| 0x01 | Error Response | 23 |
| 0x02 | Exchange MTU Request | 3 |
| 0x03 | Exchange MTU Response | 3 |
| 0x04 | Find Info Request | 12 |
| 0x05 | Find Info Response | 8 |
| 0x08 | Read By Type Request | 37 |
| 0x09 | Read By Type Response | 15 |
| 0x0a | Read Request | 6 |
| 0x0b | Read Response | 6 |
| 0x10 | Read By Group Request | 12 |
| 0x11 | Read By Group Response | 12 |
| 0x12 | Write Request | 53 |
| 0x13 | Write Response | 48 |
| 0x1b | Handle Value Notification | 181 |
| **Total** | **btatt** | **419** |

Write handle of interest: **0x000f** (32 writes with value). Notify handle: **0x000c** (175). Also CCCD-ish writes on 0x0019/0x000d.

### Full settings/control write timeline (non-handshake on 0x000f)

| Wall | t (s) | Frame | Hex | Label |
|------|------:|------:|-----|-------|
| 14:18:02 | 44.534 | 2787 | `0a1a08080310011a02280100` | **Unit Koelen** — inner `080310011a022801` (enum **1**) |
| 14:18:22 | 64.905 | 3850 | `041a09080810081a0318b10200` | setpoint **30.5°C** |
| 14:19:08 | 110.988 | 6507 | `0a1a08080310021a02280100` | **Spa** — inner `080310021a022801` (enum **1**) |
| 14:20:05 | 167.714 | 9233 | `041a09080810081a0318ac0200` | setpoint **30.0°C** |
| 14:20:30 | 192.233 | 10296 | `0a1a08080310011a02280100` | **Unit Koelen** again (enum **1**) |
| 14:20:44 | 206.146 | 11023 | `041a08080710061a02280200` | noise **Boost** (grp7 field6 enum2) |
| 14:20:51 | 214.074 | 11332 | `041a08080710061a02280300` | noise **Slim** (grp7 field6 enum3) |
| 14:20:55 | 217.434 | 11484 | `041a08080610051a02080100` | **power OFF** (grp **6** field5 bool **1**) — **NOT pump** |
| 14:21:01 | 223.577 | 11734 | `041a08080710051a02080100` | **power ON** (grp **7** field5 bool **1**) — **NOT pump** |

Handshake/selects between: `020a0100`, `042202080100` (group 1), `052202080300` (group 3 Settings) — omitted from table.

### Power vs pump (do not mislabel)

| Hex | Correct label | Why not pump |
|-----|---------------|--------------|
| `041a08080610051a02080100` | power **OFF** | group **6**, field5, **bool** `0801` — not Settings `08031005…28xx` |
| `041a08080710051a02080100` | power **ON** | group **7**, field5, **bool** `0801` — not Settings pump enum |

IO notify around power OFF: pre f05=`0801` / f04=`222b` → post f05=`0823` / f04=`2208`. No Settings `08031005` write existed in this capture.

### B-reverse gap checklist

| Target | Expected inner | Found? | Notes |
|--------|----------------|:------:|-------|
| Verwarmen (Heat enum0) | `080310011a022800` | **NO** | only Koelen `…2801` ×2 |
| Zwembad (Pool enum0) | `080310021a022800` | **NO** | only Spa `…2801` ×1 |
| Pump UIT (enum0) | `080310051a022800` | **NO** | **zero** `08031005` writes; power OFF/ON only |
| Child OFF (bool0) | `080310091a020800` | **NO** | **zero** `08031009` writes |

| Related A-state (ignore for B) | Pattern | In capture |
|-------------------------------|---------|------------|
| Cooling enum1 | `080310011a022801` | YES ×2 |
| Spa enum1 | `080310021a022801` | YES ×1 |
| Pump ON / Child ON | `…2801` / `…0801` on id5/9 | absent |

### Alternate 0-encodings / any settings enum·bool 0

Searched all ATT values:

| Pattern | Writes | Notifies |
|---------|-------:|---------:|
| `1a022800` / `2800` | 0 | 0 |
| `1a020800` / bool `0800` | 0 | 0 |
| empty / omitted / `2208` as write-0 | 0 | (IO `2208` only as state) |
| any group-3 settings write with value 0 | **0** | — |

**No settings write with enum/bool 0** in this file.

### New / interesting payloads

- Confirmed **power OFF/ON pair** in one session: `0806 1005 1a02 0801` vs `0807 1005 1a02 0801` (groups 6/7, not Settings group 3).
- Noise path on grp7 field6: Boost `2802`, Slim `2803` (already known family).
- Setpoints 30.5 (`18b102`) and 30.0 (`18ac02`) — known int×10 encoding.
- Three reconnects; 23 ATT Error Responses (attribute not found style during discovery) — noisy but expected.

### Verdict

**Not ready** for B-reverse wire confirmation (Heat0 / Pool0 / Pump0 / Child0). This capture exercised **A-side / power / noise / setpoint**, not the four reverse taps.

**Tip:** Start from Koelen+Spa+Pump AAN+Child ON, then after sniffer is running tap **only** Verwarmen, Zwembad, Pump UIT, Child OFF (one each, leave the control). Do **not** tap power — `041a080806/07 10051a02080100` is power, not pump. Expect inners `…2800` / `…0800` (or document any alternate 0 form).

## sniffer21..pcapng -- 2026-08-04

**File:** `captures/sniffer21..pcapng`  
**Size:** 334560 bytes | **Duration:** ~63.0 s | **Frames:** 4438 (nordic_ble) | **btatt:** 178  
**Procedure (intended):** without sniff set Koelen + Spa (A); start Wireshark; tap Verwarmen → Zwembad (B), maybe Pump UIT / Child OFF

### ATT counts

| Opcode | Name | Count |
|--------|------|------:|
| 0x01 | Error Response | 16 |
| 0x02 | Exchange MTU Request | 2 |
| 0x03 | Exchange MTU Response | 2 |
| 0x04 | Find Info Request | 8 |
| 0x05 | Find Info Response | 5 |
| 0x08 | Read By Type Request | 25 |
| 0x09 | Read By Type Response | 7 |
| 0x0a | Read Request | 4 |
| 0x0b | Read Response | 4 |
| 0x10 | Read By Group Request | 8 |
| 0x11 | Read By Group Response | 8 |
| 0x12 | Write Request | 29 |
| 0x13 | Write Response | 22 |
| 0x1b | Handle Value Notification | 38 |
| **Total** | **btatt** | **178** |

Write handle of interest: **0x000f** (14 writes with value). Notify handle: **0x000c** (35). CCCD-ish on 0x0019/0x000d. One mid-capture reconnect (~50–53 s).

### Full settings/control write timeline (non-handshake on 0x000f)

| Wall | t (s) | Frame | Hex | Label |
|------|------:|------:|-----|-------|
| 14:28:43 | 18.372+ | 1721+ | `042202080100` | select group 1 (×3) |
| 14:28:46 | 21.575 | 1864 | `052202080300` | select **group 3** Settings |
| **14:28:51** | **26.353** | **2249** | **`0a1a08080310011a02280100`** | UnitMode field1 — inner `080310011a022801` (**enum 1 = Koelen**, NOT Verwarmen/0) |
| **14:28:54** | **29.240** | **2513** | **`0a1a08080310021a02280100`** | PoolType field2 — inner `080310021a022801` (**enum 1 = Spa**, NOT Zwembad/0) |
| 14:29:06 | 41.474 | 3303 | `052202080400` | select **group 4** (Info / compressor schema) — not pump/child |
| 14:29:18+ | 53.35+ | 3829+ | handshake + `042202080100` + `052202080300` | reconnect; no further settings value writes |

Handshake `020a0100` omitted from table (present at connect + reconnect).

### Settings notify state around the two taps

| When | f01 UnitMode | f02 PoolType | f05 Pump | f09 Child |
|------|--------------|--------------|----------|----------|
| Pre (f2230 ~25.7 s) | `2801` Koelen | `2801` Spa | `2801` | `0823` (odd/non-`0801`) |
| After mode write (f2435) | `2821` (transitional?) | `2801` | `2801` | `0823` |
| After pool write (f2683+) | `2808` | `2819` | `2801` | `0823` |

Pump stayed `2801` for the whole session; **no** `08031005` / `08031009` writes. No power path `041a080806/07…`.

### Schema reminder (from notify f2292, not a write)

UnitMode: Heating=**0** (first / omitted tag), Cooling=`1001`, Auto=`1002`.  
PoolType: Pool=**0**, Spa=`1001`.  
So B-reverse still expects write value **`.2800`**, not `.2801`.

### B-reverse gap checklist

| Target | Expected inner | Found? | Actual |
|--------|----------------|:------:|--------|
| Verwarmen (Heat enum0) | `080310011a022800` | **NO** | only `080310011a022801` (Koelen) |
| Zwembad (Pool enum0) | `080310021a022800` | **NO** | only `080310021a022801` (Spa) |
| ANY id=1 write ≠ enum1 | — | **NO** | sole id1 write is enum **1** |
| ANY id=2 write ≠ enum1 | — | **NO** | sole id2 write is enum **1** |
| Pump UIT (enum0) | `080310051a022800` | **NO** | zero `08031005` |
| Child OFF (bool0) | `080310091a020800` | **NO** | zero `08031009` |
| Power OFF (do not call pump) | `041a08080610051a02080100` | **absent** | correctly not mislabeled |

### Alternate 0-encodings

| Pattern | Writes | Notifies (settings value) |
|---------|-------:|--------------------------:|
| `1a022800` / enum `2800` | **0** | **0** |
| `1a020800` / bool `0800` | **0** | **0** |
| empty `1a00` / `2200` / omitted-0 write | **0** | — |
| group6/7 field5 power-style | **0** | — |

**No settings write with enum/bool 0** in this file. Notify oddities (`2821`/`2808`/`2819`/`0823`) are state quirks after A-side writes — not alternate wire forms of 0.

### New / interesting

- First clear **group 4** select in this series: `052202080400` → Info/compressor notifies (`hiCompressorModel`, serial `QXFS-…`).
- Same failure mode as sniffer19/20 for mode/pool: intended B taps produced **A-side `.2801`** payloads again.

### Verdict

**Not ready** to implement B-reverse. Gaps for Heat0 / Pool0 / Pump0 / Child0 all remain; only implementable payloads here are already-known A-side Koelen/Spa (+ group4 select, not needed for those controls).

**Tip:** On the phone, confirm the **label under the control after the tap** (must show Verwarmen / Zwembad, not Koelen / Spa). Start from Koelen+Spa, sniffer running, then one deliberate tap each on **Verwarmen** and **Zwembad** (leave them selected). Separately: Pump UIT and Child OFF. Do not open Info (group4) and do not touch power. Success = writes whose inner ends in **`2800`** / **`0800`**.


## sniffer22..pcapng -- 2026-08-04

**File:** `captures/sniffer22..pcapng`  
**Size:** 756852 bytes | **Duration:** ~226.0 s | **Frames:** 11096 (nordic_ble)  
**Procedure (intended):** B-reverse after A (Verwarmen / Zwembad / Pump UIT / Child OFF).  
**What actually happened:** **A-setup again** (Koelen + Spa), twice across two BLE sessions; plus a power-style group7 write and a late group2 schema browse. **No Heat0 / Pool0.**

### ATT counts

| Opcode | Name | Count |
|--------|------|------:|
| 0x01 | Error Response | 16 |
| 0x02 | Exchange MTU Request | 2 |
| 0x03 | Exchange MTU Response | 2 |
| 0x04 | Find Info Request | 8 |
| 0x05 | Find Info Response | 6 |
| 0x08 | Read By Type Request | 25 |
| 0x09 | Read By Type Response | 10 |
| 0x0a | Read Request | 4 |
| 0x0b | Read Response | 4 |
| 0x10 | Read By Group Request | 9 |
| 0x11 | Read By Group Response | 8 |
| 0x12 | Write Request | 34 |
| 0x13 | Write Response | 31 |
| 0x1b | Handle Value Notification | 208 |
| 0x52 | Write Command | 1 |
| **Total** | **btatt** | **368** |

Write handle of interest: **0x000f** (19 writes). Notify handle: **0x000c** (204).

### Framing note (unchanged)

Phone -> device settings writes on `0x000f`: `[seq] 1a 08 [proto] 00`  
where `[proto]` = `08 03 10 <field> 1a 02 <value>`.  
Schema: UnitMode Heating=**0** / Cooling=1 / Auto=2; PoolType Pool=**0** / Spa=1.

### Full timeline (meaningful ATT value traffic)

Times are relative seconds from capture start. Two connect sessions (~10 s and ~150 s).

| t (s) | Frame | Op | Handle | Hex | Notes |
|------:|------:|----|--------|------|-------|
| 13.23-13.49 | 1695+ | WR | 0x0019/0x000d | CCCD | subscribe |
| 13.804 | 1722 | WR | 0x000f | `020a0100` | session/hello |
| 14.119 | 1756 | WR | 0x000f | `042202080100` | select group 1 |
| 14.28+ | 1789+ | NTF | 0x000c | schema | IO descriptors |
| 16.38-41.37 | 1898+ | NTF | 0x000c | IO | f05=`0823` |
| 42.365 | 2794 | WR | 0x000f | `052202080300` | select **group 3** Settings |
| 43.47-53.50 | 3015+ | NTF | 0x000c | settings **pre** | mode=`2808` pool=`2819` pump=`2801` child=`0823` |
| **54.441** | **3562** | **WR** | **0x000f** | **`0b1a08080310011a02280100`** | UnitMode -- inner `080310011a022801` (**enum 1 = Koelen**, not Heat0) |
| **57.276** | **3840** | **WR** | **0x000f** | **`0b1a08080310011a02280100`** | UnitMode retry same Koelen |
| 58.382 | 4051 | NTF | 0x000c | settings | mode -> `2801` (Cool) |
| **59.586** | **4103** | **WR** | **0x000f** | **`0b1a08080310021a02280100`** | PoolType -- inner `080310021a022801` (**enum 1 = Spa**, not Pool0) |
| 60.69+ | 4264+ | NTF | 0x000c | settings | mode=`2801` pool=`2801` (**A-setup**) pump=`2801` |
| 65.099 | 4478 | WR | 0x000f | `042202080100` | select group 1 |
| **68.459** | **4616** | **WR** | **0x000f** | **`041a08080710051a02080100`** | **Power-style** group **7** field5 **bool 1** -- NOT pump; NOT OFF |
| 68.2 -> 70.5 | 4607->4710 | NTF | 0x000c | IO | f05 `0823` -> `0801` (correlates with group7 write) |
| ~150 | | | | | **reconnect** (second session) |
| 153.75 | 7662 | WR | 0x000f | `020a0100` | hello |
| 154.02 | 7694 | WR | 0x000f | `042202080100` | select group 1 |
| 160.895 | 7997 | WR | 0x000f | `052202080300` | select group 3 |
| 162.0 | 8203 | NTF | 0x000c | settings | already Cool+Spa `2801`/`2801` |
| **163.258** | **8269** | **WR** | **0x000f** | **`0a1a08080310011a02280100`** | UnitMode again **Koelen** (`2801`) |
| **169.453** | **8755** | **WR** | **0x000f** | **`0a1a08080310021a02280100`** | PoolType again **Spa** (`2801`) |
| 174.913 | 9206 | WR | 0x000f | `042202080100` | select group 1 |
| 215+ | 10663+ | NTF | 0x000c | IO | idle; f05=`0801` |
| 220.696 | 10861 | WR | 0x000f | `052202080200` | select **group 2** (HeatpumpState schema browse) |
| 220.9+ | 10904+ | NTF | 0x000c | schema+state | status group; no settings value write |

### B-reverse gap checklist

| Target | Expected inner | Found? | Actual |
|--------|----------------|:------:|--------|
| Verwarmen (Heat enum0) | `080310011a022800` | **NO** | only `080310011a022801` (Koelen) -- full WR e.g. `0b1a08080310011a02280100` / `0a1a08080310011a02280100` |
| Zwembad (Pool enum0) | `080310021a022800` | **NO** | only `080310021a022801` (Spa) -- full WR e.g. `0b1a08080310021a02280100` / `0a1a08080310021a02280100` |
| Pump UIT (enum0) | `080310051a022800` | **NO** | zero `08031005` |
| Child OFF (bool0) | `080310091a020800` | **NO** | zero `08031009` |
| Alternate enum0 (`1a022800` / notify `2800`) | - | **NO** | 0 writes, 0 settings notifies with `2800` |
| Alternate bool0 (`1a020800`) | - | **NO** | 0 writes |
| Power OFF (do not call pump) | group6/7 field5 **bool 0** | **NO** | only group7 **bool 1**: full `041a08080710051a02080100` |

### Settings notify (session 1)

| When | f01 UnitMode | f02 PoolType | f05 Pump | f09 Child |
|------|--------------|--------------|----------|----------|
| Pre (group3) | `2808` | `2819` | `2801` | `0823` |
| After Cool writes | `2801` | `2819` | `2801` | `0823` |
| After Spa write | `2801` | `2801` | `2801` | `0823` |

Pump stayed `2801` entire capture; no child write. Session 2 repeated Cool+Spa while already on A.

### Verdict

**Not ready** for B-reverse implementation. This capture **only contains A-setup again (Koelen / Spa = `.2801`)**, plus a **power-path ON** (`041a08080710051a02080100`) and a group2 status browse. Heat0 / Pool0 / Pump0 / Child0 still missing -- **no full hex to implement for those.**

**Tip:** After starting from a known A state, watch the **on-screen labels** when tapping: must land on **Verwarmen** and **Zwembad** (not stay on / re-select Koelen / Spa). Sniffer must be running **before** those taps. Success criterion: settings WR inner ending in **`2800`** (and separately Pump UIT / Child OFF ending in **`2800`/`0800`**). Do not tap power; if you must, capture it separately and never label group6/7 field5 as pump.


## sniffer23..pcapng -- 2026-08-04

**File:** `captures/sniffer23..pcapng`  
**Size:** 373692 bytes | **Duration:** ~156.2 s | **Frames:** 5564 (nordic_ble)  
**Wall window:** 14:34:18 – 14:36:54 (+0200)  
**Procedure (intended):** B-reverse (cool→heat may be slow / late).  
**What actually happened:** Browse Settings, then **power OFF + power ON only**. **Zero** settings field writes (no id=1/2/5/9). No Heat0 / Pool0 / Pump0 / Child0 — late window checked.

**vs sniffer22:** sniffer22 already logged (A-setup Koelen/Spa `.2801` ×2 sessions + power-style group7 bool1). sniffer23 adds nothing new for B-reverse; only repeats known power path.

### ATT counts

| Opcode | Name | Count |
|--------|------|------:|
| 0x01 | Error Response | 8 |
| 0x02 | Exchange MTU Request | 1 |
| 0x03 | Exchange MTU Response | 1 |
| 0x04 | Find Info Request | 4 |
| 0x05 | Find Info Response | 2 |
| 0x08 | Read By Type Request | 12 |
| 0x09 | Read By Type Response | 4 |
| 0x0a | Read Request | 3 |
| 0x0b | Read Response | 2 |
| 0x10 | Read By Group Request | 4 |
| 0x11 | Read By Group Response | 3 |
| 0x12 | Write Request | 16 |
| 0x13 | Write Response | 14 |
| 0x1b | Handle Value Notification | 100 |
| **Total** | **btatt** | **174** |

Write handle of interest: **0x000f** (9 writes with value). Notify handle: **0x000c** (98).

### Full settings/control write timeline (non-handshake on 0x000f)

| Wall | t (s) | Frame | Hex | Label |
|------|------:|------:|-----|-------|
| 14:34:45 | 27.158+ | 905+ | `020a0100` | handshake (×2) |
| 14:34:45 | 27.421+ | 924+ | `042202080100` | select group 1 (×3) |
| 14:34:51 | 33.668 | 1202 | `052202080300` | select **group 3** Settings |
| 14:34:58 | 39.811 | 1688 | `042202080100` | back to group 1 |
| **14:35:32** | **74.516** | **2972** | **`041a08080610051a02080100`** | **Power OFF** (group **6** field5 bool **1**) — **not pump** |
| **14:35:38** | **79.871** | **3195** | **`041a08080710051a02080100`** | **Power ON** (group **7** field5 bool **1**) — **not pump** |

No further 0x000f value writes through end of capture (~156 s). Handshake omitted from “settings” meaning except noted once.

### Settings notify state (while on group 3)

| When | f01 UnitMode | f02 PoolType | f05 Pump | f09 Child |
|------|--------------|--------------|----------|----------|
| After group3 select (f1416–1699) | `2808` | `2819` | `2801` | `0823` |

Stale/transitional notify state (same quirk as sniffer21/22 pre-tap). **No** write changed these. After power OFF, IO `0a32` f05 briefly `0801`→`0823` then back on power ON — same power/IO correlation as before; settings pump stayed unread after leaving group3.

### B-reverse gap checklist

| Target | Expected inner | Found? | Actual |
|--------|----------------|:------:|--------|
| Verwarmen (Heat enum0) | `080310011a022800` | **NO** | **zero** `08031001` writes (incl. late) |
| Zwembad (Pool enum0) | `080310021a022800` | **NO** | **zero** `08031002` |
| ANY id=1 write ≠ enum1 | - | **NO** | no id=1 writes at all |
| ANY id=2 write ≠ enum1 | - | **NO** | no id=2 writes at all |
| Pump UIT (enum0) | `080310051a022800` | **NO** | zero `08031005` |
| Child OFF (bool0) | `080310091a020800` | **NO** | zero `08031009` |
| Power OFF (do not call pump) | `041a08080610051a02080100` | **YES** | correctly labeled **power**, not pump |

### Alternate 0-encodings

| Pattern | Writes | Settings notifies |
|---------|-------:|------------------:|
| `1a022800` / enum `2800` | **0** | **0** |
| `1a020800` / bool `0800` | **0** | **0** |
| empty / omitted-0 write | **0** | - |

### Verdict (file)

**Not ready.** Only implementable hex here is already-known power OFF/ON. No Heat0/Pool0 payloads.

---

## sniffer24..pcapng -- 2026-08-04

**File:** `captures/sniffer24..pcapng`  
**Size:** 121812 bytes | **Duration:** ~34.3 s | **Frames:** 1684 (nordic_ble)  
**Wall window:** 14:37:15 – 14:37:50 (+0200)  
**Procedure (intended):** B-reverse (allow slow cool→heat).  
**What actually happened:** Connect → group1 → group3 browse → **reconnect** → hello + group1 only. Capture ends. **No settings value writes whatsoever.**

**vs sniffer22:** No overlap value; 22 already had the A-side `.2801` writes. 24 is navigation/reconnect noise only.

### ATT counts

| Opcode | Name | Count |
|--------|------|------:|
| 0x01 | Error Response | 16 |
| 0x02 | Exchange MTU Request | 2 |
| 0x03 | Exchange MTU Response | 1 |
| 0x04 | Find Info Request | 8 |
| 0x05 | Find Info Response | 6 |
| 0x08 | Read By Type Request | 27 |
| 0x09 | Read By Type Response | 10 |
| 0x0a | Read Request | 4 |
| 0x0b | Read Response | 4 |
| 0x10 | Read By Group Request | 8 |
| 0x11 | Read By Group Response | 8 |
| 0x12 | Write Request | 23 |
| 0x13 | Write Response | 23 |
| 0x1b | Handle Value Notification | 23 |
| **Total** | **btatt** | **163** |

Write handle of interest: **0x000f** (10 writes with value). Notify handle: **0x000c** (19). Mid-capture reconnect ~27.9 s (frame 1341+).

### Full settings/control write timeline (non-handshake on 0x000f)

| Wall | t (s) | Frame | Hex | Label |
|------|------:|------:|-----|-------|
| 14:37:29 | 13.509+ | 472+ | `020a0100` | handshake (×2) |
| 14:37:29 | 13.772+ | 504+ | `042202080100` | select group 1 (×2) |
| 14:37:32 | 16.607 | 664 | `052202080300` | select **group 3** Settings |
| ~14:37:43 | 27.910 | 1341 | (ATT rediscovery) | **reconnect** |
| 14:37:47 | 31.271+ | 1494+ | `020a0100` | handshake (×2) |
| 14:37:47 | 31.533+ | 1526+ | `042202080100` | select group 1 (×2) |

**No** `1a08080310…` settings value writes. **No** power `041a080806/07…`. Capture ends ~2.5 s after post-reconnect group1.

### Settings notify state (group 3, pre-reconnect)

| When | f01 UnitMode | f02 PoolType | f05 Pump | f09 Child |
|------|--------------|--------------|----------|----------|
| f876–993 (~17.7–19.8 s) | `2808` | `2819` | `2801` | `0823` |

Read-only; no corresponding WR.

### B-reverse gap checklist

| Target | Expected inner | Found? | Actual |
|--------|----------------|:------:|--------|
| Verwarmen (Heat enum0) | `080310011a022800` | **NO** | zero id=1 writes |
| Zwembad (Pool enum0) | `080310021a022800` | **NO** | zero id=2 writes |
| ANY id=1 ≠ enum1 | - | **NO** | none |
| ANY id=2 ≠ enum1 | - | **NO** | none |
| Pump UIT | `080310051a022800` | **NO** | zero |
| Child OFF | `080310091a020800` | **NO** | zero |
| Power OFF (not pump) | `041a08080610051a02080100` | **absent** | correctly not mislabeled |

### Alternate 0-encodings

| Pattern | Writes | Settings notifies |
|---------|-------:|------------------:|
| `1a022800` / `2800` | **0** | **0** |
| `1a020800` / `0800` | **0** | **0** |

### Verdict (file)

**Not ready.** Empty of control payloads for B-reverse; reconnect cut the session before any taps landed on the wire.

---

## Combined verdict — sniffer23 + sniffer24

**Not ready** to implement B-reverse.

| Payload | Found? | Full hex |
|---------|:------:|----------|
| Heat0 / Verwarmen | **NO** | — |
| Pool0 / Zwembad | **NO** | — |
| Pump UIT | **NO** | — |
| Child OFF | **NO** | — |
| Power OFF (known, not pump) | sniffer23 only | `041a08080610051a02080100` |
| Power ON (known, not pump) | sniffer23 only | `041a08080710051a02080100` |

**Tip:** Keep sniffer running and wait after the cool→heat tap (writes can be late), but these two files never produced *any* UnitMode/PoolType WR — so the issue was missing taps on the wire, not only latency. Confirm the UI label becomes **Verwarmen** / **Zwembad**, then leave it; do not touch power. Success = settings WR inner ending **`2800`** / **`0800`**. sniffer22 already logged A-side `.2801` only — no need to re-mine it for Heat0.

---

## Bridge implementation note (enum labels corrected 2026-08-04)

B-reverse ATT writes for protobuf **enum 0** were never captured. Live DURACLOUD testing showed the sniffed **enum1** labels were swapped vs the app UI:

| Action | Full hex | Status |
|--------|----------|--------|
| Unit Verwarmen (id=1 enum=**1**) | `0a1a08080310011a02280100` | sniff; **live: HA Verwarmen ↔ app Verwarmen** (was wrongly labeled Koelen) |
| Unit Koelen (id=1 enum=**0**) | `0a1a08080310011a02280000` | **synthesized / still unknown**; live: HA Koelen→app stays Verwarmen (enum0 likely ignored, not “sends heat”) |
| Unit Automatisch (id=1 enum=2) | `0b1a08080310011a02280200` | sniff; live OK |
| Pool Zwembad (id=2 enum=**1**) | `0a1a08080310021a02280100` | sniff; remapped like unit (was wrongly labeled Spa) |
| Pool Spa (id=2 enum=**0**) | `0a1a08080310021a02280000` | **synthesized**; remapped by analogy — retest required |
| Pump AAN (id=5 enum=**1**) | `0b1a08080310051a02280100` | sniff; live app confirmation still needed |
| Pump Op verzoek (id=5 enum=2) | `0b1a08080310051a02280200` | sniff (sniffer8) |
| Pump Interval (id=5 enum=3) | `0b1a08080310051a02280300` | sniff (sniffer8) |
| Pump UIT (id=5 enum=0) | `0b1a08080310051a02280000` | **synthesized / still unknown** (same enum0 class as Koelen/Spa) |

Protobuf schema text in notifies still listed Heating=0 / Cooling=1 / Pool=0 / Spa=1 — that naming does **not** match Dutch DURACLOUD labels on the wire. Prefer live app confirmation over schema field names.

**Live 2026-08-04:** ESP `set_action` does call `ble_write` for unit/pool/pump (not publish-only). HA logbook shows Circulatiepomp toggles (UIT / Op verzoek / Interval) while BLE bridge was on. App “no change” for UIT fits enum0-ignore; if sniffed AAN/2/3 also fail after BLE-off app check, re-sniff pump with labels confirmed. Child lock left unimplemented.
