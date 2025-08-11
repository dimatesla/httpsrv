CC=gcc
CFLAGS=-Wall
TARGET=srv

all: $(TARGET)

$(TARGET): src/srv.c
	$(CC) $(CFLAGS) $< -o $@

install-systemd: $(BIN)
	sudo cp systemd/httpsrv.service /etc/systemd/system/
	sudo systemctl daemon-reload
	sudo systemctl enable httpsrv.service
	sudo systemctl restart httpsrv.service

clean:
	rm -f $(BIN)

