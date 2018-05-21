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
    <xsl:variable name="dmtitle" select="(//dmTitle|//dmtitle)[1]"/>
    <xsl:variable name="orig" select="(//originator|//orig)[1]"/>
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <rdf:Description>
        <dc:title>
          <xsl:apply-templates select="$dmtitle" mode="dc"/>
        </dc:title>
        <dc:creator>
          <xsl:apply-templates select="$orig" mode="dc"/>
        </dc:creator>
        <dc:subject>
          <xsl:apply-templates select="$dmtitle" mode="dc"/>
        </dc:subject>
        <dc:publisher>
          <xsl:apply-templates select="(//responsiblePartnerCompany|//rpc)[1]" mode="dc"/>
        </dc:publisher>
        <dc:contributor>
          <xsl:apply-templates select="$orig" mode="dc"/>
        </dc:contributor>
        <dc:date>
          <xsl:apply-templates select="(//issueDate|//issdate)[1]" mode="dc"/>
        </dc:date>
        <dc:type>text</dc:type>
        <dc:format>text/xml</dc:format>
        <dc:identifier>
          <xsl:apply-templates select="(//dmIdent|//dmaddres)[1]" mode="dc"/>
        </dc:identifier>
        <dc:language>
          <xsl:apply-templates select="(//language)[1]" mode="dc"/>
        </dc:language>
        <xsl:apply-templates select="//derivativeSource" mode="dc"/>
        <dc:rights>
          <xsl:value-of select="(//@securityClassification|//@class)[1]"/>
        </dc:rights>
      </rdf:Description>
      <xsl:apply-templates select="node()[not(self::rdf:Description)]"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="originator|responsiblePartnerCompany" mode="dc">
    <xsl:choose>
      <xsl:when test="enterpriseName">
        <xsl:value-of  select="enterpriseName"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="@enterpriseCode"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="orig|rpc" mode="dc">
    <xsl:choose>
      <xsl:when test="@origname|@rpcname">
        <xsl:value-of select="@origname|@rpcname"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="."/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="dmTitle" mode="dc">
    <xsl:value-of select="techName"/>
    <xsl:if test="infoName">
      <xsl:text> - </xsl:text>
      <xsl:value-of select="infoName"/>
    </xsl:if>
  </xsl:template>

  <xsl:template match="dmtitle" mode="dc">
    <xsl:value-of select="techname"/>
    <xsl:if test="infoname">
      <xsl:text> - </xsl:text>
      <xsl:value-of select="infoname"/>
    </xsl:if>
  </xsl:template>

  <xsl:template match="issueDate|issdate" mode="dc">
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

  <xsl:template match="avee" mode="dc">
    <xsl:value-of select="modelic"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="sdc"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="chapnum"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="section"/>
    <xsl:value-of select="subsect"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="subject"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="discode"/>
    <xsl:value-of select="discodev"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="incode"/>
    <xsl:value-of select="incodev"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="itemloc"/>
  </xsl:template>

  <xsl:template match="identExtension" mode="dc">
    <xsl:value-of select="@extensionProducer"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@extensionCode"/>
  </xsl:template>
  
  <xsl:template match="dmcextension" mode="dc">
    <xsl:value-of select="dmeproducer"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="dmecode"/>
  </xsl:template>

  <xsl:template match="issueInfo|issno" mode="dc">
    <xsl:value-of select="@issueNumber|@issno"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@inWork|@inwork"/>
  </xsl:template>

  <xsl:template match="language" mode="dc">
    <xsl:value-of select="@languageIsoCode|@language"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@countryIsoCode|@country"/>
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

  <xsl:template match="dmaddres" mode="dc">
    <xsl:if test="dmcextension">
      <xsl:apply-templates select="dmcextension" mode="dc"/>
      <xsl:text>_</xsl:text>
    </xsl:if>
    <xsl:apply-templates select="dmc/avee" mode="dc"/>
    <xsl:text>_</xsl:text>
    <xsl:apply-templates select="issno" mode="dc"/>
  </xsl:template>

  <xsl:template match="derivativeSource" mode="dc">
    <dc:source>
      <xsl:apply-templates/>
    </dc:source>
  </xsl:template>

</xsl:stylesheet>
