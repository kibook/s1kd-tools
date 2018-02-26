<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:template match="warningsAndCautionsRef">
    <warningsAndCautions>
      <xsl:apply-templates select="@*|node()"/>
    </warningsAndCautions>
  </xsl:template>

  <xsl:template match="warningRef">
    <xsl:variable name="win" select="@warningIdentNumber"/>
    <xsl:variable name="ident" select="//warningIdent[@warningIdentNumber = $win]"/>
    <xsl:variable name="spec" select="$ident/parent::warningSpec"/>
    <warning>
      <xsl:copy-of select="@id"/>
      <xsl:copy-of select="$spec/warningAndCautionPara"/>
    </warning>
  </xsl:template>

</xsl:stylesheet>
