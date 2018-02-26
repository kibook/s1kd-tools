<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:template match="warningsAndCautionsRef">
    <warningsAndCautions>
      <xsl:apply-templates select="@*|node()"/>
    </warningsAndCautions>
  </xsl:template>

  <xsl:template match="cautionRef">
    <xsl:variable name="cin" select="@cautionIdentNumber"/>
    <xsl:variable name="ident" select="//cautionIdent[@cautionIdentNumber = $cin]"/>
    <xsl:variable name="spec" select="$ident/parent::cautionSpec"/>
    <caution>
      <xsl:copy-of select="@id"/>
      <xsl:copy-of select="$spec/warningAndCautionPara"/>
    </caution>
  </xsl:template>

</xsl:stylesheet>
