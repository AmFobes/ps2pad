EE_BIN 	= ps2pcpad.elf 
EE_OBJS	= ps2pcpad.o networkhelper.o PS2IPS_irx.o
EE_LIBS = -lnetman -ldebug -lpatches -lps2ips -lpad -L.
#EE_LIBS = -ldebug -lps2ip -ldma -lnetman -lpatches

all: $(EE_BIN) PS2IPS_irx.c

PS2IPS_irx.c: $(PS2SDK)/iop/irx/ps2ips.irx
	bin2c $< PS2IPS_irx.c PS2IPS_irx

clean:
	rm -f $(EE_BIN) $(EE_OBJS) 

run: $(EE_BIN)
	ps2client execee host:$(EE_BIN)

reset:
	ps2client reset

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal