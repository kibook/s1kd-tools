<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  
  <xsl:template match="zoneRef">
    <xsl:variable name="zoneNumber" select="@zoneNumber"/>
    <xsl:variable name="zoneIdent" select="//zoneIdent[@zoneNumber = $zoneNumber]"/>
    <xsl:variable name="zoneSpec" select="$zoneIdent/parent::zoneSpec"/>
    <xsl:variable name="zone" select="$zoneSpec/zoneAlts/zone[1]"/>
    <zoneRef>
      <xsl:apply-templates select="@*"/>
      <xsl:choose>
        <xsl:when test="$zone">
          <xsl:apply-templates select="$zone/itemDescr"/>
          <xsl:apply-templates select="$zone/shortName"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:apply-templates select="name|shortName"/>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:apply-templates select="refs"/>
    </zoneRef>
  </xsl:template>

  <xsl:template match="itemDescr">
    <name>
      <xsl:apply-templates/>
    </name>
  </xsl:template>

</xsl:stylesheet>
