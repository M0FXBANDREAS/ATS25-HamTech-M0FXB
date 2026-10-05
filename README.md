# ATS-25 HamTech M0FXB Controller V1.7.2 BETA

RADIO-FIRST stability revision.

V1.7/V1.7.1 proved the SI4735 swept-RSSI scope works, but repeated retuning creates audible
"chuffing". That is a hardware consequence of using one SI4735 tuner for both listening and scanning.

V1.7.2 therefore:
- boots with SCOPE OFF
- performs ZERO spectrum retuning during normal listening
- retains clean AM/FM/LSB/USB reception
- keeps BFO, filters, bands, VFO A/B and saved settings
- keeps the real swept-RSSI scope as an explicit selectable function
- rotating SCOPE positive turns scanning ON; negative turns it OFF
- leaving the SCOPE control automatically turns scanning OFF and restores the listening frequency

This is intentionally not presented as simultaneous SDR spectrum + reception.
