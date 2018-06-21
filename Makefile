TARGETS=doc tools/s1kd-*

all install clean: $(TARGETS)

$(TARGETS)::
	$(MAKE) -C $@ $(MAKECMDGOALS)
