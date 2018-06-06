<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="@indexLevelOne">
    <xsl:attribute name="ref1">
      <xsl:apply-templates/>
    </xsl:attribute>
  </xsl:template>

  <xsl:template match="@indexLevelTwo">
    <xsl:attribute name="ref2">
      <xsl:apply-templates/>
    </xsl:attribute>
  </xsl:template>

  <xsl:template match="@indexLevelThree">
    <xsl:attribute name="ref3">
      <xsl:apply-templates/>
    </xsl:attribute>
  </xsl:template>

  <xsl:template match="@indexLevelFour">
    <xsl:attribute name="ref4">
      <xsl:apply-templates/>
    </xsl:attribute>
  </xsl:template>

  <xsl:template match="indexFlag">
    <indxflag>
      <xsl:apply-templates select="@*|node()"/>
    </indxflag>
  </xsl:template>

</xsl:stylesheet>
