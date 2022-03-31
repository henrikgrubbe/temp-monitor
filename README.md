## Setup

Copy `example.env` to `.env` and replace values as needed
```
cp example.env .env
```

Flash firmware
``` 
pio run -t upload 
```

Monitor serial output
```
pio device monitor
```