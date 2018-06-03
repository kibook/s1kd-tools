<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  
  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="dmlContent">
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <xsl:apply-templates select="dmlEntry">
        <xsl:sort select="dmRef/dmRefIdent/dmCode/@modelIdentCode"/>
        <xsl:sort select="dmRef/dmRefIdent/dmCode/@systemDiffCode"/>
        <xsl:sort select="dmRef/dmRefIdent/dmCode/@systemCode"/>
        <xsl:sort select="dmRef/dmRefIdent/dmCode/@subSystemCode"/>
        <xsl:sort select="dmRef/dmRefIdent/dmCode/@subSubSystemCode"/>
        <xsl:sort select="dmRef/dmRefIdent/dmCode/@assyCode"/>
        <xsl:sort select="dmRef/dmRefIdent/dmCode/@disassyCode"/>
        <xsl:sort select="dmRef/dmRefIdent/dmCode/@disassyCodeVariant"/>
        <xsl:sort select="dmRef/dmRefIdent/dmCode/@infoCode"/>
        <xsl:sort select="dmRef/dmRefIdent/dmCode/@infoCodeVariant"/>
        <xsl:sort select="dmRef/dmRefIdent/dmCode/@itemLocationCode"/>
        <xsl:sort select="dmRef/dmRefIdent/dmCode/@learnCode"/>
        <xsl:sort select="dmRef/dmRefIdent/dmCode/@learnEventCode"/>
      </xsl:apply-templates>
    </xsl:copy>
  </xsl:template>

</xsl:stylesheet>
