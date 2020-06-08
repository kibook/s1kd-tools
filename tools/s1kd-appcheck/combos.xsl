<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:output method="xml" indent="yes"/>
  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>
  <xsl:template match="propsets">
    <products>
      <xsl:apply-templates select="asserts[1]/assign"/>
    </products>
  </xsl:template>
  <xsl:template match="asserts[following-sibling::*]/assign">
    <xsl:param name="previous" select="/.."/>
    <xsl:apply-templates select="../following-sibling::asserts[1]/assign">
      <xsl:with-param name="previous" select="$previous | ."/>
    </xsl:apply-templates>
  </xsl:template>
  <xsl:template match="assign">
    <xsl:param name="previous"/>
    <product>
      <xsl:copy-of select="$previous"/>
      <xsl:copy-of select="."/>
    </product>
  </xsl:template>
</xsl:stylesheet>
