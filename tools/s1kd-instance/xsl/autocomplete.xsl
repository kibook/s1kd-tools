<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  version="1.0">
  
  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="reqCondGroup[not(*)]">
    <xsl:copy>
      <noConds/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="supportEquipDescrGroup[not(*)]">
    <noSupportEquips/>
  </xsl:template>

  <xsl:template match="supplyDescrGroup[not(*)]">
    <noSupplies/>
  </xsl:template>

  <xsl:template match="spareDescrGroup[not(*)]">
    <noSpares/>
  </xsl:template>

  <xsl:template match="safetyRqmts[not(*)]">
    <noSafety/>
  </xsl:template>

</xsl:stylesheet>
