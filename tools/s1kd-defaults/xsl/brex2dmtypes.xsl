<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:template match="/">
    <xsl:apply-templates select="//structureObjectRule[objectPath = '//@infoCode']"/>
  </xsl:template>

  <xsl:template match="structureObjectRule">
    <dmtypes>
      <xsl:apply-templates select="objectValue"/>
    </dmtypes>
  </xsl:template>

  <xsl:template match="objectValue">
    <type infoCode="{@valueAllowed}" infoName="{.}"/>
  </xsl:template>

</xsl:stylesheet>
