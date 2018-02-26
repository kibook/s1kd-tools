<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  
  <xsl:template match="referencedApplicGroupRef">
    <referencedApplicGroup>
      <xsl:apply-templates select="@*|node()"/>
    </referencedApplicGroup>
  </xsl:template>

  <xsl:template match="applicRef">
    <xsl:variable name="aiv" select="@applicIdentValue"/>
    <xsl:variable name="ident" select="//applicSpecIdent[@applicIdentValue = $aiv]"/>
    <xsl:variable name="spec" select="$ident/parent::applicSpec"/>
    <xsl:variable name="mapid" select="$spec/@applicMapRefId"/>
    <xsl:variable name="map" select="$spec/ancestor::content//applic[@id = $mapid]"/>
    <applic>
      <xsl:copy-of select="@id"/>
      <xsl:copy-of select="$map/*"/>
    </applic>
  </xsl:template>

</xsl:stylesheet>
