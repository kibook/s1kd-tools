<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  
  <xsl:template match="accessPointRef">
    <xsl:variable name="accessPointNumber" select="@accessPointNumber"/>
    <xsl:variable name="accessPointIdent" select="(//accessPointIdent[@accessPointNumber = $accessPointNumber])[1]"/>
    <xsl:variable name="accessPointSpec" select="$accessPointIdent/parent::accessPointSpec"/>
    <xsl:variable name="accessPoint" select="$accessPointSpec/accessPointAlts/accessPoint[1]"/>
    <accessPointRef>
      <xsl:apply-templates select="@*"/>
      <xsl:choose>
        <xsl:when test="$accessPoint">
          <xsl:apply-templates select="$accessPoint/name"/>
          <xsl:apply-templates select="$accessPoint/shortName"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:apply-templates select="name|shortName"/>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:apply-templates select="refs"/>
    </accessPointRef>
  </xsl:template>

</xsl:stylesheet>
