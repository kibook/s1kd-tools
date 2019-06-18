<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  version="1.0">

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="quantity">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
    <s1kd-uom_DUPLICATE>
      <xsl:text> (</xsl:text>
      <xsl:copy-of select="."/>
      <xsl:text>)</xsl:text>
    </s1kd-uom_DUPLICATE>
  </xsl:template>

  <xsl:template match="@quantityUnitOfMeasure">
    <xsl:attribute name="s1kd-uom_QUOM">
      <xsl:apply-templates/>
    </xsl:attribute>
  </xsl:template>

  <xsl:template match="@quantityTypeSpecifics">
    <xsl:attribute name="s1kd-uom_QTS">
      <xsl:apply-templates/>
    </xsl:attribute>
  </xsl:template>

</xsl:stylesheet>
