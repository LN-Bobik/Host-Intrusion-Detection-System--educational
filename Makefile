CC      = gcc
CFLAGS  = -Wall -Wextra -g
TARGET  = hids
SRCDIR  = src
LDFLAGS  = -lcrypto
SRCS    = $(SRCDIR)/main.c\
          $(SRCDIR)/logger.c $(SRCDIR)/pidfile.c $(SRCDIR)/signals.c $(SRCDIR)/monitor.c\
		  $(SRCDIR)/sha256.c $(SRCDIR)/baseline.c
OBJS    = $(SRCS:.c=.o)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(SRCDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: clean
