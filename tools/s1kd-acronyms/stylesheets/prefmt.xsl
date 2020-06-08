<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="acronym">
    <xsl:apply-templates select="acronymDefinition|acrodef"/>
    <xsl:text> (</xsl:text>
    <xsl:apply-templates select="acronymTerm|acroterm"/>
    <xsl:text>)</xsl:text>
  </xsl:template>

  <xsl:template match="acronymTerm|acroterm|acronymDefinition|acrodef">
    <xsl:apply-templates/>
  </xsl:template>

  <xsl:template match="chooseAcronym">
    <xsl:apply-templates select="acronym[1]"/>
  </xsl:template>

</xsl:stylesheet>
