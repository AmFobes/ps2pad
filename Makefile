EE_BIN 	= ps2pad.elf 
EE_OBJS	= ps2pad.o 
EE_LIBS = -lnetman -ldebug -lpatches -lps2ips -lpad -lps2gdbStub -L.
#EE_LIBS = -ldebug -lps2ip -ldma -lnetman -lpatches

all: $(EE_BIN) ps2ips.irx

ps2ips.irx:
	cp $(PS2SDK)/iop/irx/ps2ips.irx $@

clean:
	rm -f $(EE_BIN) $(EE_OBJS)

run: $(EE_BIN)
	ps2client execee host:$(EE_BIN)

reset:
	ps2client reset

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal