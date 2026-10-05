# Contributing

Bedankt voor je interesse in dit project.

## Melden van bugs

Open een issue met:

- ESP32-board type
- ESPHome-versie
- Duratech firmware / app-versie (indien bekend)
- Wat je verwachtte vs. wat er gebeurde
- Relevante ESPHome logs (mask MAC-adressen als je wilt)

## Pull requests

- Houd wijzigingen gefocust (één fix of feature per PR).
- Test op echte hardware waar mogelijk.
- Update `README.md` / `SNIFF_LOG.md` als protocolgedrag verandert.
- Voeg geen `secrets.yaml`, PCAP-bestanden of persoonlijke MAC-adressen toe.

## Protocol-onderzoek

1. Plaats captures in `captures/` (lokaal, niet committen).
2. Documenteer bevindingen in `SNIFF_LOG.md`.
3. Gebruik `python tools/analyze_pcap.py captures/jouw-bestand.pcapng --handshake`.

## Code of conduct

Wees respectvol. Dit is community-vrijwilligerswerk rond een unofficial integratie.
