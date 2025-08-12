# httpsrv — HTTP-сервер метрик для Orange Pi 3B

Простой **HTTP-сервер на C для Orange Pi 3B под Linux**. Отдаёт информацию о температуре процессора, средней нагрузке.

## Возможности
- CPU temperature (`/sys/class/thermal/.../temp`)
- Load average (`/proc/loadavg`)
- Минимум зависимостей

## Структура проекта

- `src/` - исходники (`srv.c`)
- `systemd/` - unit-файл systemd
- `docs/` - скриншоты
- `log` - лог-файл (игнорируется в git)
- `srv` - бинарник (игнорируется в git)
- `Makefile` - сборка



## Сборка
```bash
make
#or
gcc -Wall src/srv.c -o srv
```
