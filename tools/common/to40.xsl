<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"
  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
  exclude-result-prefixes="xsi">

  <xsl:variable name="schema-prefix">http://www.s1000d.org/S1000D_4-2/xml_schema_flat/</xsl:variable>
  
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

  <xsl:template match="catalogSeqNumber/@figureNumber"/>

  <xsl:template match="catalogSeqNumber/@item"/>

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
