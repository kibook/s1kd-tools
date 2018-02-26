<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  
  <xsl:template match="responsiblePartnerCompany|originator">
    <xsl:variable name="enterpriseCode" select="@enterpriseCode"/>
    <xsl:variable name="enterpriseIdent" select="//enterpriseIdent[@manufacturerCodeValue = $enterpriseCode]"/>
    <xsl:variable name="enterpriseSpec" select="$enterpriseIdent/parent::enterpriseSpec"/>
    <xsl:copy>
      <xsl:choose>
        <xsl:when test="$enterpriseSpec">
          <xsl:apply-templates select="@*"/>
          <xsl:apply-templates select="$enterpriseSpec/enterpriseName"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:apply-templates select="@*|node()"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:copy>
  </xsl:template>

</xsl:stylesheet>
