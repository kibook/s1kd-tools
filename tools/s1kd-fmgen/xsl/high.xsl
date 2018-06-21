<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <!-- Identifier for the annotation capturing the whole PM's applicability. -->
  <xsl:param name="pm-applic">app-0000</xsl:param>
  
  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="pm">
    <xsl:variable name="issueInfo" select="identAndStatusSection/pmAddress/pmIdent/issueInfo"/>
    <xsl:variable name="issueDate" select="identAndStatusSection/pmAddress/pmAddressItems/issueDate"/>
    <content>
      <referencedApplicGroup>
        <applic id="{$pm-applic}">
          <xsl:apply-templates select="identAndStatusSection/pmStatus/applic/*"/>
        </applic>
        <xsl:apply-templates select="content/referencedApplicGroup/applic"/>
      </referencedApplicGroup>
      <frontMatter>
        <frontMatterList frontMatterType="fm03">
          <xsl:apply-templates select="$issueInfo"/>
          <xsl:apply-templates select="$issueDate"/>
          <reducedPara>
            <xsl:text>The listed changes are introduced in issue </xsl:text>
            <xsl:apply-templates select="$issueInfo" mode="text"/>
            <xsl:text>, dated </xsl:text>
            <xsl:apply-templates select="$issueDate" mode="text"/>
            <xsl:text>, of this publication.</xsl:text>
          </reducedPara>
          <xsl:apply-templates select="content"/>
        </frontMatterList>
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

  <xsl:template match="content">
    <frontMatterSubList>
      <xsl:apply-templates select="//pmEntry/dmRef|//pmEntry/dmodule"/>
    </frontMatterSubList>
  </xsl:template>

  <xsl:template match="dmRef"/>

  <xsl:template match="dmodule">
    <xsl:variable name="last-app" select="ancestor-or-self::*[@applicRefId][position() = last()]"/>
    <xsl:variable name="dmodule" select="."/>
    <xsl:for-each select=".//reasonForUpdate[@updateHighlight = 1]">
      <frontMatterDmEntry>
        <xsl:apply-templates select="$dmodule/identAndStatusSection/dmStatus/@issueType"/>
        <dmRef>
          <xsl:attribute name="applicRefId">
            <xsl:choose>
              <xsl:when test="$last-app">
                <xsl:value-of select="$last-app/@applicRefId"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="$pm-applic"/>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:attribute>
          <dmRefIdent>
            <xsl:apply-templates select="$dmodule/identAndStatusSection/dmAddress/dmIdent/dmCode"/>
            <xsl:apply-templates select="$dmodule/identAndStatusSection/dmAddress/dmIdent/issueInfo"/>
            <xsl:apply-templates select="$dmodule/identAndStatusSection/dmAddress/dmIdent/language"/>
          </dmRefIdent>
          <dmRefAddressItems>
            <xsl:apply-templates select="$dmodule/identAndStatusSection/dmAddress/dmAddressItems/dmTitle"/>
            <xsl:apply-templates select="$dmodule/identAndStatusSection/dmAddress/dmAddressItems/issueDate"/>
          </dmRefAddressItems>
        </dmRef>
        <xsl:copy>
          <xsl:apply-templates select="@updateReasonType"/>
          <xsl:apply-templates select="*"/>
        </xsl:copy>
      </frontMatterDmEntry>
    </xsl:for-each>
  </xsl:template>

</xsl:stylesheet>
