#!/bin/sh

s1kd-flatten PMC-TEST-12345-00001-00_EN-CA.XML | s1kd-fmgen -f -v DMC-*.XML

xml-format -f *.XML
