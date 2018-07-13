<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:template match="/">
    <defaults>
      <xsl:apply-templates select="//structureObjectRule"/>
    </defaults>
  </xsl:template>

  <xsl:template match="structureObjectRule">
    <xsl:variable name="ident">
      <xsl:choose>
        <xsl:when test="objectPath = '//@countryIsoCode'">countryIsoCode</xsl:when>
        <xsl:when test="objectPath = '//@languageIsoCode'">languageIsoCode</xsl:when>
        <xsl:when test="objectPath = '//@modelIdentCode'">modelIdentCode</xsl:when>
        <xsl:when test="objectPath = '//originator/enterpriseName'">originator</xsl:when>
        <xsl:when test="objectPath = '//originator/@enterpriseCode'">originatorCode</xsl:when>
        <xsl:when test="objectPath = '//responsiblePartnerCompany/enterpriseName'">responsiblePartnerCompany</xsl:when>
        <xsl:when test="objectPath = '//responsiblePartnerCompany/@enterpriseCode'">responsiblePartnerCompanyCode</xsl:when>
        <xsl:when test="objectPath = '//@securityClassification'">securityClassification</xsl:when>
        <xsl:when test="objectPath = '//@skillLevelCode">skillLevelCode</xsl:when>
      </xsl:choose>
    </xsl:variable>
    <xsl:if test="$ident != '' and objectValue">
      <default ident="{$ident}" value="{objectValue[1]/@valueAllowed}"/>
    </xsl:if>
  </xsl:template>

</xsl:stylesheet>
