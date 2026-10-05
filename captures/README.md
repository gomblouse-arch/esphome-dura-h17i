# BLE captures

Plaats hier je Wireshark/nRF Sniffer `.pcapng`-bestanden (niet in git — te groot).

Analyseer met:

```bash
python tools/analyze_pcap.py captures/sniffer25.pcapng
```

Of geef een map:

```bash
python tools/analyze_pcap.py captures/
```

Zie `SNIFF_LOG.md` voor gedocumenteerde bevindingen per capture.
