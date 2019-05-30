#!/bin/sh

DM=DMC-TEST-A-00-00-00-00A-040A-D_000-01_EN-CA.XML

echo "Schema validation:"
s1kd-validate -v $DM
echo
echo "Applicability validation:"
s1kd-appcheck -v $DM
