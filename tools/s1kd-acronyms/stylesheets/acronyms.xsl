<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  
  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="/">
    <acronyms>
      <xsl:apply-templates select="//acronym"/>
    </acronyms>
  </xsl:template>

  <xsl:template match="acronym">
    <xsl:copy>
      <xsl:apply-templates select="@acronymType|@acrotype"/>
      <xsl:apply-templates select="acronymTerm|acroterm"/>
      <xsl:apply-templates select="acronymDefinition|acrodef"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="@acrotype">
    <xsl:attribute name="acronymType">
      <xsl:apply-templates/>
    </xsl:attribute>
  </xsl:template>

  <xsl:template match="acroterm">
    <acronymTerm>
      <xsl:apply-templates/>
    </acronymTerm>
  </xsl:template>

  <xsl:template match="acrodef">
    <acronymDefinition>
      <xsl:apply-templates/>
    </acronymDefinition>
  </xsl:template>

</xsl:stylesheet>
