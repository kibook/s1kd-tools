S1KD_INSTANCE=../../s1kd-instance

DMS+=DMC-AJ-A-35-13-51-00A-040A-A_000-01_EN-CA.XML
DMS+=DMC-AJ-A-35-13-51-00B-520A-A_000-01_EN-CA.XML
DMS+=DMC-AJ-A-35-13-51-00C-520A-A_000-01_EN-CA.XML

.PHONY: all clean

all: customerA customerB

customer%: $(DMS)
	$(S1KD_INSTANCE) -Qw -O $@ -s config:prodattr=$* $+

clean:
	rm -rf customerA customerB
