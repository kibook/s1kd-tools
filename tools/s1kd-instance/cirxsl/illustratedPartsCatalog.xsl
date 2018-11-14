<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  
  <xsl:template match="supportEquipDescr[catalogSeqNumberRef]|supplyDescr[catalogSeqNumberRef]|spareDescr[catalogSeqNumberRef]">
    <xsl:variable name="modelIdentCode" select="catalogSeqNumberRef/@modelIdentCode"/>
    <xsl:variable name="systemDiffCode" select="catalogSeqNumberRef/@systemDiffCode"/>
    <xsl:variable name="systemCode" select="catalogSeqNumberRef/@systemCode"/>
    <xsl:variable name="subSystemCode" select="catalogSeqNumberRef/@subSystemCode"/>
    <xsl:variable name="subSubSystemCode" select="catalogSeqNumberRef/@subSubSystemCode"/>
    <xsl:variable name="assyCode" select="catalogSeqNumberRef/@assyCode"/>
    <xsl:variable name="figureNumber" select="catalogSeqNumberRef/@figureNumber"/>
    <xsl:variable name="figureNumberVariant" select="catalogSeqNumberRef/@figureNumberVariant"/>
    <xsl:variable name="item" select="catalogSeqNumberRef/@item"/>
    <xsl:variable name="itemVariant" select="catalogSeqNumberRef/@itemVariant"/>
    <xsl:variable name="itemSeqNumberValue" select="catalogSeqNumberRef/@itemSeqNumberValue"/>
    <xsl:variable name="itemLocationCode" select="catalogSeqNumberRef/@itemLocationCode"/>
    <xsl:variable name="dmCode" select="//dmodule[2]//dmCode[1]"/>
    <xsl:variable name="catalogSeqNumber" select="
      (//catalogSeqNumber[
        (not($modelIdentCode) or
         $modelIdentCode = @modelIdentCode or
         $modelIdentCode = $dmCode/@modelIdentCode) and
        (not($systemDiffCode) or
         $systemDiffCode = @systemDiffCode or
         $systemDiffCode = $dmCode/@systemDiffCode) and
        (not($systemCode) or
         $systemCode = @systemCode or
         $systemCode = $dmCode/@systemCode) and
        (not($subSystemCode) or
         $subSystemCode = @subSystemCode or
         $subSystemCode = $dmCode/@subSystemCode) and
        (not($subSubSystemCode) or
         $subSubSystemCode = @subSubSystemCode or
         $subSubSystemCode = $dmCode/@subSubSystemCode) and
        (not($assyCode) or
         $assyCode = @assyCode or
         $assyCode = $dmCode/@assyCode) and
        @figureNumber = $figureNumber and
        (not($figureNumberVariant) or
         $figureNumberVariant = @figureNumberVariant or
         $figureNumberVariant = $dmCode/@disassyCodeVariant) and
        @item = $item and
        (not($itemVariant) or $itemVariant = @itemVariant)
      ])[1]"/>
    <xsl:copy>
      <xsl:choose>
        <xsl:when test="$catalogSeqNumber">
          <xsl:apply-templates select="@*"/>
          <xsl:apply-templates select="$catalogSeqNumber/itemSeqNumber/partSegment/itemIdentData/descrForPart"/>
          <xsl:apply-templates select="$catalogSeqNumber/itemSeqNumber/partSegment/itemIdentData/shortName"/>
          <xsl:apply-templates select="catalogSeqNumberRef|natoStockNumber|identNumber|partRef|functionalItemRef|materialSetRef"/>
          <xsl:apply-templates select="reqQuantity"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:apply-templates select="@*|node()"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="descrForPart">
    <name>
      <xsl:apply-templates/>
    </name>
  </xsl:template>

</xsl:stylesheet>
