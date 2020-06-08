<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="pm">
    <content>
      <frontMatter>
        <xsl:apply-templates select="identAndStatusSection"/>
      </frontMatter>
    </content>
  </xsl:template>
  
  <xsl:template match="identAndStatusSection">
    <frontMatterTitlePage>
      <xsl:apply-templates select=".//pmTitle"/>
      <xsl:apply-templates select=".//shortPmTitle"/>
      <xsl:apply-templates select=".//pmCode"/>
      <xsl:apply-templates select=".//issueInfo"/>
      <xsl:apply-templates select=".//issueDate"/>
      <xsl:apply-templates select=".//security"/>
      <xsl:apply-templates select=".//derivativeClassification"/>
      <xsl:apply-templates select=".//dataRestrictions"/>
      <xsl:apply-templates select=".//responsiblePartnerCompany"/>
    </frontMatterTitlePage>
  </xsl:template>

</xsl:stylesheet>
