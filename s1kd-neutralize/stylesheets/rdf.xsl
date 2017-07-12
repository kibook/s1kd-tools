<?xml version="1.0"?>
<xsl:stylesheet
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
  xmlns:dc="http://www.purl.org/dc/elements/1.1/"
  version="1.0">
  
  <xsl:template match="node()|@*">
    <xsl:copy>
      <xsl:apply-templates select="node()|@*"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="dmodule">
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <rdf:Description>
        <dc:title>
          <xsl:apply-templates select="//dmAddressItems/dmTitle" mode="dc"/>
        </dc:title>
        <dc:creator>
          <xsl:apply-templates select="//dmStatus/originator" mode="dc"/>
        </dc:creator>
        <dc:subject>
          <xsl:apply-templates select="//dmAddressItems/dmTitle" mode="dc"/>
        </dc:subject>
        <dc:publisher>
          <xsl:apply-templates select="//dmStatus/responsiblePartnerCompany" mode="dc"/>
        </dc:publisher>
        <dc:contributor>
          <xsl:apply-templates select="//dmStatus/originator" mode="dc"/>
        </dc:contributor>
        <dc:date>
          <xsl:apply-templates select="//dmAddressItems/issueDate" mode="dc"/>
        </dc:date>
        <dc:type>text</dc:type>
        <dc:format>text/xml</dc:format>
        <dc:identifier>
          <xsl:apply-templates select="//dmIdent" mode="dc"/>
        </dc:identifier>
        <dc:language>
          <xsl:apply-templates select="//dmIdent/language" mode="dc"/>
        </dc:language>
        <dc:source>
          <xsl:apply-templates select="//derivativeSource" mode="dc"/>
        </dc:source>
        <dc:rights>
          <xsl:value-of select="//dmStatus/security/@securityClassification"/>
        </dc:rights>
      </rdf:Description>
      <xsl:apply-templates select="node()[not(self::rdf:Description)]"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="originator|responsiblePartnerCompany" mode="dc">
    <xsl:value-of  select="enterpriseName"/>
  </xsl:template>

  <xsl:template match="dmTitle" mode="dc">
    <xsl:value-of select="techName"/>
    <xsl:if test="infoName">
      <xsl:text> - </xsl:text>
      <xsl:value-of select="infoName"/>
    </xsl:if>
  </xsl:template>

  <xsl:template match="issueDate" mode="dc">
    <xsl:value-of select="@year"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@month"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@day"/>
  </xsl:template>

<xsl:template match="dmCode" mode="dc">
    <xsl:value-of select="@modelIdentCode"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@systemDiffCode"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@systemCode"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@subSystemCode"/>
    <xsl:value-of select="@subSubSystemCode"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@assyCode"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@disassyCode"/>
    <xsl:value-of select="@disassyCodeVariant"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@infoCode"/>
    <xsl:value-of select="@infoCodeVariant"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@itemLocationCode"/>
    <xsl:if test="@learnCode">
      <xsl:text>-</xsl:text>
      <xsl:value-of select="@learnCode"/>
      <xsl:value-of select="@learnEventCode"/>
    </xsl:if>
  </xsl:template>

  <xsl:template match="identExtension" mode="dc">
    <xsl:value-of select="@extensionProducer"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@extensionCode"/>
  </xsl:template>

  <xsl:template match="issueInfo" mode="dc">
    <xsl:value-of select="@issueNumber"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@inWork"/>
  </xsl:template>

  <xsl:template match="language" mode="dc">
    <xsl:value-of select="@languageIsoCode"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@countryIsoCode"/>
  </xsl:template>

  <xsl:template match="dmIdent" mode="dc">
    <xsl:if test="identExtension">
      <xsl:apply-templates select="identExtension" mode="dc"/>
      <xsl:text>_</xsl:text>
    </xsl:if>
    <xsl:apply-templates select="dmCode" mode="dc"/>
    <xsl:text>_</xsl:text>
    <xsl:apply-templates select="issueInfo" mode="dc"/>
  </xsl:template>

</xsl:stylesheet>
