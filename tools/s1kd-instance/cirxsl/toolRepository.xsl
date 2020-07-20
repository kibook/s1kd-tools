<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="supportEquipDescr[toolRef]">
    <xsl:variable name="toolNumber" select="toolRef/@toolNumber"/>
    <xsl:variable name="manufacturerCodeValue" select="toolRef/@manufacturerCodeValue"/>
    <xsl:variable name="toolIdent" select="(//toolIdent[@toolNumber = $toolNumber and (not($manufacturerCodeValue) or @manufacturerCodeValue = $manufacturerCodeValue)])[1]"/>
    <xsl:variable name="toolSpec" select="$toolIdent/parent::toolSpec"/>
    <supportEquipDescr>
      <xsl:choose>
        <xsl:when test="$toolSpec">
          <xsl:apply-templates select="@*"/>
          <xsl:apply-templates select="$toolSpec/itemIdentData/descrForPart" mode="name"/>
          <xsl:apply-templates select="$toolSpec/itemIdentData/shortName"/>
          <xsl:apply-templates select="catalogSeqNumberRef|natoStockNumber|identNumber|toolRef|materialSetRef"/>
          <xsl:apply-templates select="reqQuantity"/>
          <xsl:apply-templates select="remarks|footnoteRemarks"/>
          <xsl:apply-templates select="embeddedSupportEquipDescr"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:apply-templates select="@*|node()"/>
        </xsl:otherwise>
      </xsl:choose>
    </supportEquipDescr>
  </xsl:template>

  <xsl:template match="descrForPart" mode="name">
    <name>
      <xsl:apply-templates/>
    </name>
  </xsl:template>

</xsl:stylesheet>
