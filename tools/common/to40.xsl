<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  exclude-result-prefixes="xsi">

  <xsl:variable name="schema-prefix">http://www.s1000d.org/S1000D_6/xml_schema_flat/</xsl:variable>
  
  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="@xsi:noNamespaceSchemaLocation">
    <xsl:attribute name="xsi:noNamespaceSchemaLocation">
      <xsl:text>http://www.s1000d.org/S1000D_4-0/xml_schema_flat/</xsl:text>
      <xsl:value-of select="substring-after(., $schema-prefix)"/>
    </xsl:attribute>
  </xsl:template>

  <xsl:template match="dmTitle">
    <xsl:copy>
      <xsl:apply-templates select="@*"/>
      <xsl:apply-templates select="techName|infoName"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="infoName">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
      <xsl:apply-templates select="parent::dmTitle/infoNameVariant"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="infoNameVariant">
    <xsl:text>, </xsl:text>
    <xsl:apply-templates select="node()"/>
  </xsl:template>

  <xsl:template match="commentStatus">
    <xsl:copy>
      <xsl:apply-templates select="security"/>
      <xsl:apply-templates select="commentPriority"/>
      <xsl:apply-templates select="commentResponse"/>
      <xsl:apply-templates select="commentRefs"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="ddnStatus">
    <xsl:copy>
      <xsl:apply-templates select="security"/>
      <xsl:apply-templates select="authorization"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="dmlStatus">
    <xsl:copy>
      <xsl:apply-templates select="security"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="pmStatus">
    <xsl:copy>
      <xsl:apply-templates select="security"/>
      <xsl:apply-templates select="responsiblePartnerCompany"/>
      <xsl:apply-templates select="applic"/>
      <xsl:apply-templates select="qualityAssurance"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="dmlEntry">
    <dmEntry>
      <xsl:apply-templates select="@*|node()"/>
    </dmEntry>
  </xsl:template>

  <xsl:template match="catalogSeqNumber">
    <xsl:copy>
      <xsl:apply-templates select="@id"/>
      <xsl:apply-templates select="@changeMark"/>
      <xsl:apply-templates select="@changeType"/>
      <xsl:apply-templates select="@reasonForUpdateRefIds"/>
      <xsl:attribute name="catalogSeqNumberValue">
        <xsl:choose>
          <xsl:when test="@systemCode and @subSystemCode and @subSubSystemCode and @assyCode">
            <xsl:value-of select="@systemCode"/>
            <xsl:value-of select="@subSystemCode"/>
            <xsl:value-of select="@subSubSystemCode"/>
            <xsl:value-of select="@assyCode"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="'      '"/>
          </xsl:otherwise>
        </xsl:choose>
        <xsl:value-of select="@figureNumber"/>
        <xsl:choose>
          <xsl:when test="@figureNumberVariant">
            <xsl:value-of select="@figureNumberVariant"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="' '"/>
          </xsl:otherwise>
        </xsl:choose>
        <xsl:value-of select="@item"/>
        <xsl:choose>
          <xsl:when test="@itemVariant">
            <xsl:value-of select="@itemVariant"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="' '"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>
      <xsl:apply-templates select="@indenture"/>
      <xsl:apply-templates select="@securityClassification"/>
      <xsl:apply-templates select="@commercialClassification"/>
      <xsl:apply-templates select="@caveat"/>
      <xsl:apply-templates select="node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="catalogSeqNumberRef">
    <xsl:copy>
      <xsl:apply-templates select="@id"/>
      <xsl:apply-templates select="@changeMark"/>
      <xsl:apply-templates select="@changeType"/>
      <xsl:apply-templates select="@reasonForUpdateRefIds"/>
      <xsl:apply-templates select="@initialProvisioningProjectValue"/>
      <xsl:apply-templates select="@responsiblePartnerCompanyCode"/>
      <xsl:apply-templates select="@authorityName"/>
      <xsl:apply-templates select="@authorityDocument"/>
      <xsl:apply-templates select="@securityClassification"/>
      <xsl:apply-templates select="@commercialClassification"/>
      <xsl:apply-templates select="@caveat"/>
      <xsl:attribute name="catalogSeqNumberValue">
        <xsl:choose>
          <xsl:when test="@systemCode and @subSystemCode and @subSubSystemCode and @assyCode">
            <xsl:value-of select="@systemCode"/>
            <xsl:value-of select="@subSystemCode"/>
            <xsl:value-of select="@subSubSystemCode"/>
            <xsl:value-of select="@assyCode"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="'      '"/>
          </xsl:otherwise>
        </xsl:choose>
        <xsl:value-of select="@figureNumber"/>
        <xsl:choose>
          <xsl:when test="@figureNumberVariant">
            <xsl:value-of select="@figureNumberVariant"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="' '"/>
          </xsl:otherwise>
        </xsl:choose>
        <xsl:value-of select="@item"/>
        <xsl:choose>
          <xsl:when test="@itemVariant">
            <xsl:value-of select="@itemVariant"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="' '"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>
      <xsl:apply-templates select="@itemSeqNumberValue"/>
      <xsl:apply-templates select="node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="itemSeqNumber">
    <itemSequenceNumber>
      <xsl:apply-templates select="@*"/>
      <xsl:apply-templates select="quantityPerNextHigherAssy"/>
      <xsl:apply-templates select="partRef"/>
      <partIdentSegment>
        <descrForPart/>
      </partIdentSegment>
      <locationRcmdSegment>
        <locationRcmd>
          <service/>
          <sourceMaintRecoverability/>
        </locationRcmd>
      </locationRcmdSegment>
    </itemSequenceNumber>
  </xsl:template>

  <xsl:template match="partRef">
    <xsl:apply-templates select="@*"/>
  </xsl:template>

  <xsl:template match="partRef/@manufacturerCodeValue">
    <manufacturerCode>
      <xsl:apply-templates/>
    </manufacturerCode>
  </xsl:template>

  <xsl:template match="partRef/@partNumberValue">
    <partNumber>
      <xsl:apply-templates/>
    </partNumber>
  </xsl:template>

</xsl:stylesheet>
