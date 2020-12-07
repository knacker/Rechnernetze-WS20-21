
bin_PROGRAMS = timers local_dgram_client local_dgram_server udp_client udp_server

timers_SOURCES = timers.c
timers_CFLAGS = -I$(top_srcdir)/src
timers_LDADD = $(top_srcdir)/src/xdt/libxdt.a

local_dgram_server_SOURCES = local_dgram.h local_dgram_server.c
local_dgram_client_SOURCES = local_dgram.h local_dgram_client.c

udp_server_SOURCES = udp_server.c
udp_client_SOURCES = udp_client.c

