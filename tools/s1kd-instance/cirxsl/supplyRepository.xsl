<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>
  
  <xsl:template match="supplyDescr[supplyRef]">
    <xsl:variable name="supplyNumber" select="supplyRef/@supplyNumber"/>
    <xsl:variable name="supplyNumberType" select="supplyRef/@supplyNumberType"/>
    <xsl:variable name="supplySpec" select="(//supplySpec[supplyIdent[@supplyNumber = $supplyNumber and @supplyNumberType = $supplyNumberType]])[1]"/>
    <supplyDescr>
      <xsl:choose>
        <xsl:when test="$supplySpec">
          <xsl:apply-templates select="@*"/>
          <xsl:apply-templates select="$supplySpec/name"/>
          <xsl:apply-templates select="$supplySpec/shortName"/>
          <xsl:apply-templates select="catalogSeqNumberRef|identNumber|supplyRef|supplyRqmtRef|materialSetRef"/>
          <xsl:apply-templates select="$supplySpec/natoStockNumber"/>
          <xsl:apply-templates select="reqQuantity"/>
          <xsl:apply-templates select="remarks|footnoteRemarks"/>
          <xsl:apply-templates select="embeddedSupplyDescr"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:apply-templates select="@*|node()"/>
        </xsl:otherwise>
      </xsl:choose>
    </supplyDescr>
  </xsl:template>

</xsl:stylesheet>
