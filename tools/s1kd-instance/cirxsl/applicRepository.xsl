<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>
  
  <xsl:template match="referencedApplicGroupRef">
    <referencedApplicGroup>
      <xsl:apply-templates select="@*|node()"/>
    </referencedApplicGroup>
  </xsl:template>

  <xsl:template match="applicRef">
    <xsl:variable name="applicIdentValue" select="@applicIdentValue"/>
    <xsl:variable name="applicSpec" select="(//applicSpec[applicSpecIdent[@applicIdentValue = $applicIdentValue]])[1]"/>
    <xsl:variable name="applicMapRefId" select="$applicSpec/@applicMapRefId"/>
    <xsl:variable name="applic" select="$applicSpec/ancestor::content//applic[@id = $applicMapRefId]"/>
    <applic>
      <xsl:apply-templates select="@id"/>
      <xsl:apply-templates select="$applic/node()"/>
    </applic>
  </xsl:template>

</xsl:stylesheet>
