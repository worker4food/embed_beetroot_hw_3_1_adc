## ESP-IDF Project

```bash
$ idf.py --version
ESP-IDF v6.1
```

## Schematics

```mermaid
graph LR
    VCC["3.3V"] --> LDR["LDR"]
    LDR --> VOUT["gpio08"]
    VOUT --> R1["1kOhm"]
    R1 --> GND1["GND"]

    GPIO47["gpio47"] --> LED["Blue LED"]
    LED --> R2["220Ohm"]
    R2 --> GND2["GND"]
```

## Commands

```shell
# Build project
$ idf.py app

# Flash firmware
$ idf.py flash

# Monitor
$ idf.py monitor

# Clean build files
$ idf.py fullclean
```
