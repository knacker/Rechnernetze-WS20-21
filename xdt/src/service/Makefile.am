
bin_PROGRAMS = service

service_SOURCES = main.c \
                  pdu.h pdu.c \
                  queue.h queue.c \
                  errors.h errors.c \
                  service.h service.c \
                  sender.h sender.c \
                  receiver.h receiver.c

service_CFLAGS = -I$(top_srcdir)/src
service_LDADD = $(top_srcdir)/src/xdt/libxdt.a
