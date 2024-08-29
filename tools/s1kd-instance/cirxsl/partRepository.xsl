<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>
  
  <xsl:template match="spareDescr[partRef]">
    <xsl:variable name="partNumberValue" select="partRef/@partNumberValue"/>
    <xsl:variable name="manufacturerCodeValue" select="partRef/@manufacturerCodeValue"/>
    <xsl:variable name="partSpec" select="(//partSpec[partIdent[@partNumberValue = $partNumberValue and @manufacturerCodeValue = $manufacturerCodeValue]])[1]"/>
    <spareDescr>
      <xsl:choose>
        <xsl:when test="$partSpec">
          <xsl:apply-templates select="@*"/>
          <xsl:apply-templates select="$partSpec/itemIdentData/descrForPart" mode="name"/>
          <xsl:apply-templates select="$partSpec/itemIdentData/shortName"/>
          <xsl:apply-templates select="catalogSeqNumberRef|identNumber|partRef|functionalItemRef|materialSetRef"/>
          <xsl:apply-templates select="$partSpec/itemIdentData/natoStockNumber"/>
          <xsl:apply-templates select="reqQuantity"/>
          <xsl:apply-templates select="remarks|footnoteRemarks"/>
          <xsl:apply-templates select="embeddedSpareDescr"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:apply-templates select="@*|node()"/>
        </xsl:otherwise>
      </xsl:choose>
    </spareDescr>
  </xsl:template>

  <xsl:template match="itemSeqNumber[partRef]">
    <xsl:variable name="partNumberValue" select="partRef/@partNumberValue"/>
    <xsl:variable name="manufacturerCodeValue" select="partRef/@manufacturerCodeValue"/>
    <xsl:variable name="partSpec" select="(//partSpec[partIdent[@partNumberValue = $partNumberValue and @manufacturerCodeValue = $manufacturerCodeValue]])[1]"/>
    <itemSeqNumber>
      <xsl:choose>
        <xsl:when test="$partSpec">
          <xsl:apply-templates select="@*"/>
          <xsl:apply-templates select="quantityPerNextHigherAssy|totalQuantity|removalOrInstallationQuantity|partRef"/>
          <partSegment>
            <xsl:copy-of select="$partSpec/itemIdentData"/>
            <xsl:copy-of select="$partSpec/procurementData"/>
            <xsl:copy-of select="$partSpec/techData"/>
            <xsl:copy-of select="$partSpec/partRefGroup"/>
          </partSegment>
          <xsl:apply-templates select="partLocationSegment|applicabilitySegment|categoryOneContainerLocation|locationRcmdSegment|functionalItemRef|ilsNumber|changeAuthorityData|genericPartDataGroup"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:apply-templates select="@*|node()"/>
        </xsl:otherwise>
      </xsl:choose>
    </itemSeqNumber>
  </xsl:template>

  <xsl:template match="descrForPart" mode="name">
    <name>
      <xsl:apply-templates/>
    </name>
  </xsl:template>

</xsl:stylesheet>
