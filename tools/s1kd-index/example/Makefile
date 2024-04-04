upissue = s1kd-upissue
index = s1kd-index

dm1 = DMC-EX-A-00-00-00-00A-040A-D_000-01_EN-CA.XML
dm2 = DMC-EX-A-00-00-00-00A-040A-D_000-02_EN-CA.XML

$(dm2): $(dm1)
	$(upissue) $(dm1)
	$(index) -fi $(dm2)
clean:
	rm -f $(dm2)
