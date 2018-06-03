<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:param name="infoCode"/>
  
  <xsl:variable name="modelIdentCode" select="//@modelIdentCode[1]"/>
  <xsl:variable name="responsiblePartnerCompany" select="//responsiblePartnerCompany[1]"/>

  <xsl:template match="/">
    <dmlContent>
      <xsl:apply-templates select="//snsSystem"/>
    </dmlContent>
  </xsl:template>

  <xsl:template name="create-entry">
    <dmlEntry>
      <dmRef>
        <dmRefIdent>
          <dmCode>
            <xsl:attribute name="modelIdentCode">
              <xsl:value-of select="$modelIdentCode"/>
            </xsl:attribute>
            <xsl:attribute name="systemDiffCode">A</xsl:attribute>
            <xsl:choose>
              <xsl:when test="self::snsAssy">
                <xsl:apply-templates select="ancestor::snsSystem" mode="code"/>
                <xsl:apply-templates select="ancestor::snsSubSystem" mode="code"/>
                <xsl:apply-templates select="ancestor::snsSubSubSystem" mode="code"/>
                <xsl:apply-templates select="." mode="code"/>
              </xsl:when>
              <xsl:when test="self::snsSubSubSystem">
                <xsl:apply-templates select="ancestor::snsSystem" mode="code"/>
                <xsl:apply-templates select="ancestor::snsSubSystem" mode="code"/>
                <xsl:apply-templates select="." mode="code"/>
                <xsl:attribute name="assyCode">00</xsl:attribute>
              </xsl:when>
              <xsl:when test="self::snsSubSystem">
                <xsl:apply-templates select="ancestor::snsSystem" mode="code"/>
                <xsl:apply-templates select="." mode="code"/>
                <xsl:attribute name="subSubSystemCode">0</xsl:attribute>
                <xsl:attribute name="assyCode">00</xsl:attribute>
              </xsl:when>
              <xsl:when test="self::snsSystem">
                <xsl:apply-templates select="." mode="code"/>
                <xsl:attribute name="subSystemCode">0</xsl:attribute>
                <xsl:attribute name="subSubSystemCode">0</xsl:attribute>
                <xsl:attribute name="assyCode">00</xsl:attribute>
              </xsl:when>
            </xsl:choose>
            <xsl:attribute name="disassyCode">00</xsl:attribute>
            <xsl:attribute name="disassyCodeVariant">A</xsl:attribute>
            <xsl:attribute name="infoCode">
              <xsl:value-of select="$infoCode"/>
            </xsl:attribute>
            <xsl:attribute name="infoCodeVariant">A</xsl:attribute>
            <xsl:attribute name="itemLocationCode">D</xsl:attribute>
          </dmCode>
        </dmRefIdent>
      </dmRef>
      <xsl:copy-of select="$responsiblePartnerCompany"/>
    </dmlEntry>
  </xsl:template>

  <xsl:template match="snsSystem|snsSubSystem|snsSubSubSystem|snsAssy">
    <xsl:call-template name="create-entry"/>
  </xsl:template>

  <xsl:template match="snsSystem[snsSubSystem]">
    <xsl:apply-templates select="snsSubSystem"/>
  </xsl:template>

  <xsl:template match="snsSubSystem[snsSubSubSystem]">
    <xsl:apply-templates select="snsSubSubSystem"/>
  </xsl:template>

  <xsl:template match="snsSubSubSystem[snsAssy]">
    <xsl:apply-templates select="snsAssy"/>
  </xsl:template>

  <xsl:template match="snsAssy" mode="code">
    <xsl:attribute name="assyCode">
      <xsl:value-of select="snsCode"/>
    </xsl:attribute>
  </xsl:template>

  <xsl:template match="snsSubSubSystem" mode="code">
    <xsl:attribute name="subSubSystemCode">
      <xsl:value-of select="snsCode"/>
    </xsl:attribute>
  </xsl:template>

  <xsl:template match="snsSubSystem" mode="code">
    <xsl:attribute name="subSystemCode">
      <xsl:value-of select="snsCode"/>
    </xsl:attribute>
  </xsl:template>

  <xsl:template match="snsSystem" mode="code">
    <xsl:attribute name="systemCode">
      <xsl:value-of select="snsCode"/>
    </xsl:attribute>
  </xsl:template>

</xsl:stylesheet>
