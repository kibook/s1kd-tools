.PHONY: all clean

all: Test.exe

Test.exe: ../../S1kdTools.cs Test.cs
	mono-csc -unsafe -out:$@ $+

clean:
	rm -f Test.exe
