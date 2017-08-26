<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  
  <xsl:template match="/">
    <acronyms>
      <xsl:apply-templates/>
    </acronyms>
  </xsl:template>
  
  <xsl:template match="*">
    <xsl:apply-templates select="*"/>
  </xsl:template>

  <xsl:template match="acronym">
    <xsl:copy>
      <xsl:apply-templates select="@acronymType|acronymTerm|acronymDefinition"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="acronym/acronymTerm">
    <xsl:copy>
      <xsl:apply-templates/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="acronymDefinition">
    <xsl:copy>
      <xsl:apply-templates/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="@acronymType">
    <xsl:copy>
      <xsl:apply-templates/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="acronymTerm/subScript|acronymTerm/superScript|acronymDefinition/subScript|acronymDefinition/superScript">
    <xsl:copy>
      <xsl:apply-templates/>
    </xsl:copy>
  </xsl:template>

</xsl:stylesheet>
