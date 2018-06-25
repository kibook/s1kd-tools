TARGETS=doc tools/s1kd-*

all clean install uninstall: $(TARGETS)

$(TARGETS)::
	$(MAKE) -C $@ $(MAKECMDGOALS)
