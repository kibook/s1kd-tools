<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  version="1.0">

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="s1kd-uom_DUPLICATE">
    <xsl:if test="descendant::processing-instruction()[name() = 's1kd-uom_CONVERTED']">
      <xsl:apply-templates select="node()"/>
    </xsl:if>
  </xsl:template>

  <xsl:template match="processing-instruction()[name() = 's1kd-uom_CONVERTED']"/>

  <xsl:template match="@s1kd-uom_QUOM">
    <xsl:attribute name="quantityUnitOfMeasure">
      <xsl:apply-templates/>
    </xsl:attribute>
  </xsl:template>

  <xsl:template match="@s1kd-uom_QTS">
    <xsl:attribute name="quantityTypeSpecifics">
      <xsl:apply-templates/>
    </xsl:attribute>
  </xsl:template>

</xsl:stylesheet>
