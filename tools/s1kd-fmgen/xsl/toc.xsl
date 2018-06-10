<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- Identifier for the annotation capturing the whole DM's applicablility. -->
  <xsl:param name="dm-applic">app-0000</xsl:param>

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="pm">
    <xsl:variable name="issueInfo" select="identAndStatusSection/pmAddress/pmIdent/issueInfo"/>
    <xsl:variable name="issueDate" select="identAndStatusSection/pmAddress/pmAddressItems/issueDate"/>
    <content>
      <xsl:call-template name="referenced.applic.group"/>
      <frontMatter>
        <frontMatterTableOfContent>
          <xsl:apply-templates select="$issueInfo"/>
          <xsl:apply-templates select="$issueDate"/>
          <reducedPara>
            <xsl:text>The listed documents are included in issue </xsl:text>
            <xsl:apply-templates select="$issueInfo" mode="text"/>
            <xsl:text>, dated </xsl:text>
            <xsl:apply-templates select="$issueDate" mode="text"/>
            <xsl:text>, of this publication.</xsl:text>
          </reducedPara>
          <xsl:apply-templates select="content"/>
        </frontMatterTableOfContent>
      </frontMatter>
    </content>
  </xsl:template>

  <xsl:template match="issueInfo" mode="text">
    <xsl:value-of select="@issueNumber"/>
    <xsl:if test="@inWork != '00'">
      <xsl:text>-</xsl:text>
      <xsl:value-of select="@inWork"/>
    </xsl:if>
  </xsl:template>

  <xsl:template match="issueDate" mode="text">
    <xsl:value-of select="@year"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@month"/>
    <xsl:text>-</xsl:text>
    <xsl:value-of select="@day"/>
  </xsl:template>

  <xsl:template name="referenced.applic.group">
    <referencedApplicGroup>
      <applic id="{$dm-applic}">
        <xsl:apply-templates select="identAndStatusSection/pmStatus/applic/*"/>
      </applic>
      <xsl:apply-templates select="content/referencedApplicGroup/applic"/>
    </referencedApplicGroup>
  </xsl:template>

  <xsl:template match="pmTitle">
    <title>
      <xsl:apply-templates select="@*|node()"/>
    </title>
  </xsl:template>

  <xsl:template match="content">
    <tocList>
      <xsl:apply-templates select="pmEntry"/>
    </tocList>
  </xsl:template>

  <xsl:template match="pmEntry">
    <tocEntry>
      <xsl:apply-templates select="@*|node()"/>
    </tocEntry>
  </xsl:template>

  <xsl:template match="pmEntryTitle">
    <title>
      <xsl:apply-templates select="@*|node()"/>
    </title>
  </xsl:template>

  <xsl:template match="dmRef">
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <xsl:if test="not(@applicRefId)">
        <xsl:attribute name="applicRefId">
          <xsl:value-of select="$dm-applic"/>
        </xsl:attribute>
      </xsl:if>
      <xsl:apply-templates select="node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="dmodule">
    <dmRef>
      <xsl:attribute name="applicRefId">
        <xsl:choose>
          <xsl:when test="@applicRefId">
            <xsl:value-of select="@applicRefId"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="$dm-applic"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>
      <dmRefIdent>
        <xsl:apply-templates select="identAndStatusSection/dmAddress/dmIdent/dmCode"/>
        <xsl:apply-templates select="identAndStatusSection/dmAddress/dmIdent/issueInfo"/>
        <xsl:apply-templates select="identAndStatusSection/dmAddress/dmIdent/language"/>
      </dmRefIdent>
      <dmRefAddressItems>
        <xsl:apply-templates select="identAndStatusSection/dmAddress/dmAddressItems/dmTitle"/>
        <xsl:apply-templates select="identAndStatusSection/dmAddress/dmAddressItems/issueDate"/>
      </dmRefAddressItems>
    </dmRef>
  </xsl:template>

</xsl:stylesheet>
