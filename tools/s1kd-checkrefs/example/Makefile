all: output.log DMC-CHCKREFS-A-00-00-02-00A-040A-D_000-02_EN-CA.XML DMC-CHCKREFS-A-00-00-01-00A-040A-D_000-02_EN-CA.XML

output.log:
	# Check all data modules for invalid references
	s1kd-checkrefs -c DMC-*.XML > output.log 2>&1

DMC-CHCKREFS-A-00-00-01-00A-040A-D_000-02_EN-CA.XML: DMC-CHCKREFS-A-00-00-01-00A-040A-D_000-01_EN-CA.XML
	s1kd-upissue DMC-CHCKREFS-A-00-00-01-00A-040A-D_000-01_EN-CA.XML
	# Check latest issues of data modules and update address items in references
	s1kd-ls -l | xargs s1kd-checkrefs -ceu externalPubs.xml

DMC-CHCKREFS-A-00-00-02-00A-040A-D_000-02_EN-CA.XML: DMC-CHCKREFS-A-00-00-02-00A-040A-D_000-01_EN-CA.XML
	s1kd-upissue DMC-CHCKREFS-A-00-00-02-00A-040A-D_000-01_EN-CA.XML
	# Check latest issues of data modules and update address items in references
	s1kd-ls -l | xargs s1kd-checkrefs -cu

clean:
	rm -f output.log
	rm -f DMC-CHCKREFS-A-00-00-02-00A-040A-D_000-02_EN-CA.XML
	rm -f DMC-CHCKREFS-A-00-00-01-00A-040A-D_000-02_EN-CA.XML
