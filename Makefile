all:
	$(MAKE) -C s1kd-brexcheck
	$(MAKE) -C s1kd-dmls
	$(MAKE) -C s1kd-dmref
	$(MAKE) -C s1kd-instance
	$(MAKE) -C s1kd-metadata
	$(MAKE) -C s1kd-neutralize
	$(MAKE) -C s1kd-newdm
	$(MAKE) -C s1kd-newpm
	$(MAKE) -C s1kd-syncrefs
	$(MAKE) -C s1kd-upissue
	$(MAKE) -C s1kd-validate

clean:
	$(MAKE) -C s1kd-brexcheck clean
	$(MAKE) -C s1kd-dmls clean
	$(MAKE) -C s1kd-dmref clean
	$(MAKE) -C s1kd-instance clean
	$(MAKE) -C s1kd-metadata clean
	$(MAKE) -C s1kd-neutralize clean
	$(MAKE) -C s1kd-newdm clean
	$(MAKE) -C s1kd-newpm clean
	$(MAKE) -C s1kd-syncrefs clean
	$(MAKE) -C s1kd-upissue clean
	$(MAKE) -C s1kd-validate clean

install:
	$(MAKE) -C s1kd-brexcheck install
	$(MAKE) -C s1kd-dmls install
	$(MAKE) -C s1kd-dmref install
	$(MAKE) -C s1kd-instance install
	$(MAKE) -C s1kd-metadata install
	$(MAKE) -C s1kd-neutralize install
	$(MAKE) -C s1kd-newdm install
	$(MAKE) -C s1kd-newpm install
	$(MAKE) -C s1kd-syncrefs install
	$(MAKE) -C s1kd-upissue install
	$(MAKE) -C s1kd-validate install
