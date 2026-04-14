# brink_ot2 (ESPHome) – Brink Excelent 400 / Renovent HR OpenTherm

ESPHome external component dla rekuperatora Brink (Excelent 400 / Renovent HR) po OpenTherm, testowane pod Wemos D1 mini (ESP8266) + OpenTherm shield (Ihor Melnyk / diyless).

## Sprzęt
- Wemos D1 mini (ESP8266)
- OpenTherm shield (master)
- Połączenie 2-przewodowe z Brink (OpenTherm)

## Użycie w ESPHome
Najprościej uruchomić przez pliki z katalogu `examples/`.

### Kompilacja lokalna
```bash
pip install -U esphome
esphome compile examples/wemos_d1_mini_basic.yaml
```

## Obsługiwane encje (na start)
- Number: sterowanie wentylacją (OT ID 71) 0–100%
- Sensors: temperatury OT ID 80–83
- Sensor: przepływ (TSP 52/53 jako 2 bajty)
- Binary sensor: filtr (TSP 13)
- Sensor + text sensor: bypass status (TSP 55)
- Sensors: CPID/CPOD (TSP 64/65 i 66/67)
- Sensors: U1/U2/U3 (TSP 0/1, 2/3, 4/5)
- Sensors: U4/U5 (TSP 6 i 7, dzielone przez 2 => °C)
- Sensor: I1 (TSP 9, -100)

## Uwagi
W repo `brink_openhab` opisane są dodatkowe TSP i zachowanie bypassu (workaround z okresowym połączeniem). Jeśli chcesz, można dodać przełącznik/tryb do takiej pracy również w ESPHome.
