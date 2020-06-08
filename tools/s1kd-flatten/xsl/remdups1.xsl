<?xml version="1.0" encoding="UTF-8"?>
<xsl:transform
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:xi="http://www.w3.org/2001/XInclude"
  version="1.0">

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="pmEntry/dmRef">
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <xsl:attribute name="IDENT">
        <xsl:apply-templates select="dmRefIdent" mode="text"/>
      </xsl:attribute>
      <xsl:apply-templates select="node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="pmEntry/dmodule|/publication/dmodule">
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <xsl:attribute name="IDENT">
        <xsl:apply-templates select="identAndStatusSection/dmAddress/dmIdent|idstatus/dmaddres" mode="text"/>
      </xsl:attribute>
      <xsl:apply-templates select="node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="pmentry/refdm">
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <xsl:attribute name="IDENT">
        <xsl:apply-templates select="dmeextension" mode="text"/>
        <xsl:apply-templates select="dmc/avee" mode="text"/>
        <xsl:apply-templates select="issno" mode="text"/>
        <xsl:apply-templates select="language" mode="text"/>
      </xsl:attribute>
      <xsl:apply-templates select="node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="dmIdent|dmRefIdent" mode="text">
    <xsl:apply-templates select="identExtension" mode="text"/>
    <xsl:apply-templates select="dmCode" mode="text"/>
    <xsl:apply-templates select="issueInfo" mode="text"/>
    <xsl:apply-templates select="language" mode="text"/>
  </xsl:template>

  <xsl:template match="dmaddres" mode="text">
    <xsl:apply-templates select="dmcextension" mode="text"/>
    <xsl:apply-templates select="dmc/avee" mode="text"/>
    <xsl:apply-templates select="issno" mode="text"/>
    <xsl:apply-templates select="language" mode="text"/>
  </xsl:template>

  <xsl:template match="identExtension" mode="text">
    <xsl:value-of select="@extensionProducer"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@extensionCode"/>
    <xsl:text>-</xsl:text>
  </xsl:template>

  <xsl:template match="dmCode|avee" mode="text">
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

  <xsl:template match="issueInfo|issno" mode="text">
    <xsl:text>_</xsl:text>
    <xsl:value-of select="@issueNumber|@issno"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@inWork|@inwork"/>
  </xsl:template>

  <xsl:template match="language" mode="text">
    <xsl:text>_</xsl:text>
    <xsl:value-of select="@languageIsoCode|@language"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@countryIsoCode|@country"/>
  </xsl:template>

  <xsl:template match="pmEntry/pmRef">
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <xsl:attribute name="IDENT">
        <xsl:apply-templates select="pmRefIdent" mode="text"/>
      </xsl:attribute>
      <xsl:apply-templates select="node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="pmRefIdent" mode="text">
    <xsl:apply-templates select="identExtension" mode="text"/>
    <xsl:apply-templates select="pmCode" mode="text"/>
    <xsl:apply-templates select="issueInfo" mode="text"/>
    <xsl:apply-templates select="language" mode="text"/>
  </xsl:template>

  <xsl:template match="pmCode" mode="text">
    <xsl:value-of select="@modelIdentCode"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@pmIssuer"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@pmNumber"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@pmVolume"/>
  </xsl:template>

  <xsl:template match="pmEntry/externalPubRef">
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <xsl:attribute name="IDENT">
        <xsl:apply-templates select="externalPubRefIdent" mode="text"/>
      </xsl:attribute>
      <xsl:apply-templates select="node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="externalPubRefIdent" mode="text">
    <xsl:value-of select="(externalPubCode|externalPubTitle)[1]"/>
  </xsl:template>

  <xsl:template match="pmEntry/xi:include|/publication/xi:include">
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <xsl:attribute name="IDENT">
        <xsl:value-of select="@href"/>
      </xsl:attribute>
      <xsl:apply-templates select="node()"/>
    </xsl:copy>
  </xsl:template>

</xsl:transform>
