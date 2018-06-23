<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

  <xsl:template match="functionalItemRef">
    <xsl:variable name="functionalItemNumber" select="@functionalItemNumber"/>
    <xsl:variable name="functionalItemType" select="@functionalItemType"/>
    <xsl:variable name="installationIdent" select="@installationIdent"/>
    <xsl:variable name="contextIdent" select="@contextIdent"/>
    <xsl:variable name="manufacturerCodeValue" select="@manufacturerCodeValue"/>
    <xsl:variable name="itemOriginator" select="@itemOriginator"/>
    <xsl:variable name="functionalItemIdent" select="
      //functionalItemIdent[
        $functionalItemNumber = @functionalItemNumber and
        (not($functionalItemType) or $functionalItemType = @functionalItemType) and
        (not($installationIdent) or $installationIdent = @installationIdent) and
        (not($contextIdent) or $contextIdent = @contextIdent) and
        (not($manufacturerCodeValue) or $manufacturerCodeValue = @manufacturerCodeValue) and
        (not($itemOriginator) or $itemOriginator = @itemOriginator)
      ]"/>
    <xsl:variable name="functionalItemSpec" select="$functionalItemIdent/parent::functionalItemSpec"/>
    <xsl:variable name="functionalItem" select="$functionalItemSpec/functionalItemAlts/functionalItem[1]"/>
    <functionalItemRef>
      <xsl:choose>
        <xsl:when test="$functionalItemSpec">
          <xsl:apply-templates select="@id"/>
          <xsl:apply-templates select="$functionalItemIdent/@functionalItemNumber"/>
          <xsl:apply-templates select="$functionalItemIdent/@functionalItemType"/>
          <xsl:apply-templates select="$functionalItemIdent/@installationIdent"/>
          <xsl:apply-templates select="$functionalItemIdent/@contextIdent"/>
          <xsl:apply-templates select="$functionalItemIdent/@manufacturerCodeValue"/>
          <xsl:apply-templates select="$functionalItemIdent/@itemOriginator"/>
          <xsl:choose>
            <xsl:when test="$functionalItem/name">
              <xsl:apply-templates select="$functionalItem/name"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:apply-templates select="$functionalItemSpec/name"/>
            </xsl:otherwise>
          </xsl:choose>
          <xsl:choose>
            <xsl:when test="$functionalItem/shortName">
              <xsl:apply-templates select="$functionalItem/shortName"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:apply-templates select="$functionalItemSpec/shortName"/>
            </xsl:otherwise>
          </xsl:choose>
          <xsl:apply-templates select="$functionalItemSpec/refs"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:apply-templates select="@*|node()"/>
        </xsl:otherwise>
      </xsl:choose>
    </functionalItemRef>
  </xsl:template>

</xsl:stylesheet>
