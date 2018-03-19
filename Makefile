all:
	$(MAKE) -C doc
	$(MAKE) -C s1kd-acronyms
	$(MAKE) -C s1kd-addicn
	$(MAKE) -C s1kd-aspp
	$(MAKE) -C s1kd-brexcheck
	$(MAKE) -C s1kd-checkrefs
	$(MAKE) -C s1kd-defaults
	$(MAKE) -C s1kd-dmls
	$(MAKE) -C s1kd-ref
	$(MAKE) -C s1kd-dmrl
	$(MAKE) -C s1kd-flatten
	$(MAKE) -C s1kd-instance
	$(MAKE) -C s1kd-metadata
	$(MAKE) -C s1kd-neutralize
	$(MAKE) -C s1kd-newcom
	$(MAKE) -C s1kd-newddn
	$(MAKE) -C s1kd-newdm
	$(MAKE) -C s1kd-newdml
	$(MAKE) -C s1kd-newimf
	$(MAKE) -C s1kd-newpm
	$(MAKE) -C s1kd-refls
	$(MAKE) -C s1kd-syncrefs
	$(MAKE) -C s1kd-transform
	$(MAKE) -C s1kd-upissue
	$(MAKE) -C s1kd-validate

clean:
	$(MAKE) -C doc clean
	$(MAKE) -C s1kd-acronyms clean
	$(MAKE) -C s1kd-addicn clean
	$(MAKE) -C s1kd-aspp clean
	$(MAKE) -C s1kd-brexcheck clean
	$(MAKE) -C s1kd-checkrefs clean
	$(MAKE) -C s1kd-defaults clean
	$(MAKE) -C s1kd-dmls clean
	$(MAKE) -C s1kd-ref clean
	$(MAKE) -C s1kd-dmrl clean
	$(MAKE) -C s1kd-flatten clean
	$(MAKE) -C s1kd-instance clean
	$(MAKE) -C s1kd-metadata clean
	$(MAKE) -C s1kd-neutralize clean
	$(MAKE) -C s1kd-newcom clean
	$(MAKE) -C s1kd-newddn clean
	$(MAKE) -C s1kd-newdm clean
	$(MAKE) -C s1kd-newdml clean
	$(MAKE) -C s1kd-newimf clean
	$(MAKE) -C s1kd-newpm clean
	$(MAKE) -C s1kd-refls clean
	$(MAKE) -C s1kd-syncrefs clean
	$(MAKE) -C s1kd-transform clean
	$(MAKE) -C s1kd-upissue clean
	$(MAKE) -C s1kd-validate clean

install:
	$(MAKE) -C doc install
	$(MAKE) -C s1kd-acronyms install
	$(MAKE) -C s1kd-addicn install
	$(MAKE) -C s1kd-aspp install
	$(MAKE) -C s1kd-brexcheck install
	$(MAKE) -C s1kd-checkrefs install
	$(MAKE) -C s1kd-defaults install
	$(MAKE) -C s1kd-dmls install
	$(MAKE) -C s1kd-ref install
	$(MAKE) -C s1kd-dmrl install
	$(MAKE) -C s1kd-flatten install
	$(MAKE) -C s1kd-instance install
	$(MAKE) -C s1kd-metadata install
	$(MAKE) -C s1kd-neutralize install
	$(MAKE) -C s1kd-newcom install
	$(MAKE) -C s1kd-newddn install
	$(MAKE) -C s1kd-newdm install
	$(MAKE) -C s1kd-newdml install
	$(MAKE) -C s1kd-newimf install
	$(MAKE) -C s1kd-newpm install
	$(MAKE) -C s1kd-refls install
	$(MAKE) -C s1kd-syncrefs install
	$(MAKE) -C s1kd-transform install
	$(MAKE) -C s1kd-upissue install
	$(MAKE) -C s1kd-validate install
