<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  version="1.0">

  <xsl:param name="prefix"> (</xsl:param>
  <xsl:param name="postfix">)</xsl:param>

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
      <xsl:value-of select="$prefix"/>
      <xsl:copy-of select="."/>
      <xsl:value-of select="$postfix"/>
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
