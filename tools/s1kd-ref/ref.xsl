<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  version="1.0">

  <xsl:output method="text"/>

  <xsl:template match="*">
    <xsl:apply-templates select="*"/>
  </xsl:template>

  <xsl:template match="commentRef|dmRef|refdm|dmlRef|pmRef|refpm"/>

  <xsl:template match="dmIdent|dmaddres">
    <xsl:choose>
      <xsl:when test="identExtension">DME-</xsl:when>
      <xsl:otherwise>DMC-</xsl:otherwise>
    </xsl:choose>
    <xsl:apply-templates select="identExtension|dmcextension"/>
    <xsl:apply-templates select="dmCode|dmc"/>
    <xsl:apply-templates select="issueInfo|issno"/>
    <xsl:apply-templates select="language"/>
  </xsl:template>

  <xsl:template match="identExtension|dmcextension">
    <xsl:value-of select="@extensionProducer|dmeproducer"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@extensionCode|dmecode"/>
    <xsl:text>-</xsl:text>
  </xsl:template>

  <xsl:template match="dmCode|avee">
    <xsl:value-of select="@modelIdentCode|modelic"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@systemDiffCode|sdc"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@systemCode|chapnum"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@subSystemCode|section"/>
    <xsl:value-of select="@subSubSystemCode|subsect"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@assyCode|subject"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@disassyCode|discode"/>
    <xsl:value-of select="@disassyCodeVariant|discodev"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@infoCode|incode"/>
    <xsl:value-of select="@infoCodeVariant|incodev"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@itemLocationCode|itemloc"/>
    <xsl:if test="@learnCode">
      <xsl:text>-</xsl:text>
      <xsl:value-of select="@learnCode"/>
      <xsl:value-of select="@learnEventCode"/>
    </xsl:if>
  </xsl:template>

  <xsl:template match="issueInfo|issno">
    <xsl:text>_</xsl:text>
    <xsl:value-of select="@issueNumber|@issno"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@inWork|@inwork"/>
  </xsl:template>

  <xsl:template match="language">
    <xsl:text>_</xsl:text>
    <xsl:value-of select="@languageIsoCode|@language"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@countryIsoCode|@country"/>
  </xsl:template>

  <xsl:template match="pmIdent|pmaddres">
    <xsl:choose>
      <xsl:when test="identExtension">PME-</xsl:when>
      <xsl:otherwise>PMC-</xsl:otherwise>
    </xsl:choose>
    <xsl:apply-templates select="identExtension"/>
    <xsl:apply-templates select="pmCode|pmc"/>
    <xsl:apply-templates select="issueInfo|issno"/>
    <xsl:apply-templates select="language"/>
  </xsl:template>

  <xsl:template match="pmc">
    <xsl:value-of select="@modelIdentCode|modelic"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@pmIssuer|pmissuer"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@pmNumber|pmnumber"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@pmVolume|pmvolume"/>
  </xsl:template>

  <xsl:template match="scormContentPackageIdent">
    <xsl:choose>
      <xsl:when test="identExtension">SME-</xsl:when>
      <xsl:otherwise>SMC-</xsl:otherwise>
    </xsl:choose>
    <xsl:apply-templates select="identExtension"/>
    <xsl:apply-templates select="scormContentPackageCode"/>
    <xsl:apply-templates select="issueInfo"/>
    <xsl:apply-templates select="language"/>
  </xsl:template>

  <xsl:template match="scormContentPackageCode">
    <xsl:value-of select="@modelIdentCode"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@scormContentPackageIssuer"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@scormContentPackageNumber"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@scormContentPackageVolume"/>
  </xsl:template>

  <xsl:template match="commentIdent|cstatus">
    <xsl:text>COM-</xsl:text>
    <xsl:apply-templates select="commentCode|ccode"/>
    <xsl:apply-templates select="language"/>
  </xsl:template>

  <xsl:template match="commentCode|ccode">
    <xsl:value-of select="@modelIdentCode|modelic"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@senderIdent|sendid"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@yearOfDataIssue|diyear"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@seqNumber|seqnum"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@commentType|ctype/@type"/>
  </xsl:template>

  <xsl:template match="dmlIdent|dml[dmlc]">
    <xsl:text>DML-</xsl:text>
    <xsl:apply-templates select="dmlCode|dmlc"/>
    <xsl:apply-templates select="issueInfo|issno"/>
  </xsl:template>

  <xsl:template match="dmlCode|dmlc">
    <xsl:value-of select="@modelIdentCode|modelic"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@senderIdent|sendid"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@dmlType|dmltype/@type"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@yearOfDataIssue|diyear"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@seqNumber|seqnum"/>
  </xsl:template>

  <xsl:template match="imfIdent">
    <xsl:text>ICN-</xsl:text>
    <xsl:value-of select="imfCode/@imfIdentIcn"/>
  </xsl:template>

</xsl:stylesheet>
